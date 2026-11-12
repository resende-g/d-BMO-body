#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Gabriel Almeida Resende
# SPDX-License-Identifier: CERN-OHL-P-2.0
"""Gera envelopes paramétricos em STEP a partir de components.yaml.

Regra central, aplicada em código: um componente só vira geometria quando TODAS
as dimensões que os seus sólidos usam têm status CONFIRMADO, valor numérico
positivo em milímetros e pelo menos uma fonte de fabricante com URL. Qualquer
outro caso é recusado, com o motivo escrito no relatório e no manifesto.

Os sólidos são caixas envolventes (envelopes), não peças detalhadas.

Uso:
    python generate_models.py            # gera em ./out e imprime o relatório
    python generate_models.py --check    # só mostra o que seria gerado ou recusado
    python generate_models.py --out DIR  # outro diretório de saída

Dependências: PyYAML e cadquery (ver mechanical/README.md). A opção --check
não precisa do cadquery.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import yaml

HERE = Path(__file__).resolve().parent
DEFAULT_REGISTER = HERE / "components.yaml"
DEFAULT_OUT = HERE / "out"

STATUS_CONFIRMED = "CONFIRMADO"
STATUS_NOMINAL = "NOMINAL"
STATUS_PENDING = "PENDENTE"
ALLOWED_STATUSES = (STATUS_CONFIRMED, STATUS_NOMINAL, STATUS_PENDING)

# Único conjunto de status que pode virar geometria. Não ampliar sem decisão
# registrada do mantenedor (ver mechanical/README.md, "Regra de recusa").
MODELABLE_STATUSES = frozenset({STATUS_CONFIRMED})

MANUFACTURER_ORIGIN = "fabricante"
LENGTH_UNIT = "mm"
SPDX_LICENSE = "CERN-OHL-P-2.0"
SPDX_COPYRIGHT = "2026 Gabriel Almeida Resende"
MANIFEST_NAME = "manifest.json"

# Chaves de um sólido que apontam para dimensões do registro.
SOLID_SIZE_KEYS = ("x", "y", "z")
SOLID_OPTIONAL_DIMENSION_KEYS = ("y_flush_max_with", "grow")
SOLID_KNOWN_KEYS = frozenset(("name",) + SOLID_SIZE_KEYS + SOLID_OPTIONAL_DIMENSION_KEYS)

NAME_PATTERN = re.compile(r"^[a-z0-9_]+$")
BBOX_TOLERANCE_MM = 1e-6

# Marca posta só por confirmed_length() e box_specs(). build_solid() recusa
# objetos montados à mão, que não passaram pela verificação do registro.
_ISSUED = object()


class RegisterError(ValueError):
    """O registro está malformado ou incoerente."""


class EnvelopeRefused(RuntimeError):
    """O componente não pode ser modelado. `reasons` lista os motivos."""

    def __init__(self, component_id: str, reasons: list[str]):
        self.component_id = component_id
        self.reasons = list(reasons)
        super().__init__(f"{component_id}: " + "; ".join(self.reasons))


@dataclass(frozen=True)
class ConfirmedLength:
    """Comprimento em mm que passou pela verificação de status e de fonte.

    Só `confirmed_length()` cria instâncias; a geometria só aceita este tipo.
    """

    component_id: str
    key: str
    value: float
    sources: tuple[str, ...]
    token: object = field(default=None, repr=False, compare=False)


@dataclass(frozen=True)
class BoxSpec:
    """Caixa alinhada aos eixos, no referencial local do componente."""

    component_id: str
    name: str
    size: tuple[float, float, float]
    center: tuple[float, float, float]
    dimensions_used: tuple[ConfirmedLength, ...]
    token: object = field(default=None, repr=False, compare=False)

    @property
    def file_stem(self) -> str:
        return f"{self.component_id}__{self.name}"

    @property
    def bounds(self) -> tuple[tuple[float, float], tuple[float, float], tuple[float, float]]:
        return tuple(
            (c - s / 2.0, c + s / 2.0) for c, s in zip(self.center, self.size)
        )  # type: ignore[return-value]


# --------------------------------------------------------------------------
# Leitura e validação do registro
# --------------------------------------------------------------------------


def load_register(path: Path | str = DEFAULT_REGISTER) -> dict[str, Any]:
    with open(path, encoding="utf-8") as handle:
        register = yaml.safe_load(handle)
    if not isinstance(register, dict):
        raise RegisterError("o registro precisa ser um mapa YAML")
    for key in ("sources", "components"):
        if not isinstance(register.get(key), dict):
            raise RegisterError(f"falta a seção '{key}' no registro")
    return register


def _is_positive_number(value: Any) -> bool:
    return (
        isinstance(value, (int, float))
        and not isinstance(value, bool)
        and math.isfinite(value)
        and value > 0
    )


def _manufacturer_sources(dimension: dict[str, Any], sources: dict[str, Any]) -> list[str]:
    found = []
    for source_id in dimension.get("source") or []:
        source = sources.get(source_id)
        if not isinstance(source, dict):
            continue
        url = source.get("url")
        if (
            source.get("origin") == MANUFACTURER_ORIGIN
            and isinstance(url, str)
            and url.startswith("https://")
            and source.get("accessed")
        ):
            found.append(source_id)
    return found


def validate_register(register: dict[str, Any]) -> list[str]:
    """Devolve a lista de incoerências do registro (vazia se estiver íntegro)."""
    problems: list[str] = []
    sources = register["sources"]
    default_unit = register.get("units", LENGTH_UNIT)
    if default_unit != LENGTH_UNIT:
        problems.append(f"units: esperado '{LENGTH_UNIT}', lido '{default_unit}'")

    for source_id, source in sources.items():
        if not isinstance(source, dict) or "origin" not in source:
            problems.append(f"sources.{source_id}: falta 'origin'")

    for component_id, component in register["components"].items():
        prefix = f"components.{component_id}"
        if not NAME_PATTERN.match(str(component_id)):
            problems.append(f"{prefix}: identificador fora do padrão [a-z0-9_]")
        dimensions = component.get("dimensions")
        if not isinstance(dimensions, dict) or not dimensions:
            problems.append(f"{prefix}: falta 'dimensions'")
            continue
        for key, dimension in dimensions.items():
            where = f"{prefix}.dimensions.{key}"
            status = dimension.get("status")
            value = dimension.get("value")
            listed = dimension.get("source") or []
            if status not in ALLOWED_STATUSES:
                problems.append(f"{where}: status inválido {status!r}")
                continue
            for source_id in listed:
                if source_id not in sources:
                    problems.append(f"{where}: fonte desconhecida {source_id!r}")
            if status == STATUS_PENDING:
                if value is not None:
                    problems.append(f"{where}: PENDENTE não pode ter valor")
            else:
                if not _is_positive_number(value):
                    problems.append(f"{where}: {status} exige valor numérico positivo")
                if not listed:
                    problems.append(f"{where}: {status} exige ao menos uma fonte")
            if status == STATUS_CONFIRMED and not _manufacturer_sources(dimension, sources):
                problems.append(
                    f"{where}: CONFIRMADO exige fonte de fabricante com URL https e data de acesso"
                )
        for solid in (component.get("cad") or {}).get("solids") or []:
            name = solid.get("name")
            if not isinstance(name, str) or not NAME_PATTERN.match(name):
                problems.append(f"{prefix}.cad: nome de sólido inválido {name!r}")
            unknown = set(solid) - SOLID_KNOWN_KEYS
            if unknown:
                problems.append(f"{prefix}.cad.{name}: chaves desconhecidas {sorted(unknown)}")
            for size_key in SOLID_SIZE_KEYS:
                if size_key not in solid:
                    problems.append(f"{prefix}.cad.{name}: falta a chave '{size_key}'")
    return problems


# --------------------------------------------------------------------------
# Regra de recusa
# --------------------------------------------------------------------------


def dimension_problems(
    component: dict[str, Any], key: str, register: dict[str, Any]
) -> list[str]:
    """Motivos pelos quais a dimensão `key` não pode virar geometria."""
    dimensions = component.get("dimensions") or {}
    dimension = dimensions.get(key)
    if not isinstance(dimension, dict):
        return [f"{key}: dimensão ausente do registro"]
    status = dimension.get("status")
    if status not in MODELABLE_STATUSES:
        return [f"{key}: status {status}"]
    problems = []
    if not _is_positive_number(dimension.get("value")):
        problems.append(f"{key}: sem valor numérico positivo")
    unit = dimension.get("unit", register.get("units", LENGTH_UNIT))
    if unit != LENGTH_UNIT:
        problems.append(f"{key}: unidade '{unit}' não é comprimento em mm")
    if not _manufacturer_sources(dimension, register["sources"]):
        problems.append(f"{key}: sem fonte de fabricante com URL")
    return problems


def required_dimension_keys(component: dict[str, Any]) -> list[str]:
    """Dimensões usadas pelos sólidos declarados, na ordem em que aparecem."""
    keys: list[str] = []
    for solid in (component.get("cad") or {}).get("solids") or []:
        for field in SOLID_SIZE_KEYS + SOLID_OPTIONAL_DIMENSION_KEYS:
            key = solid.get(field)
            if key is not None and key not in keys:
                keys.append(key)
    return keys


def refusal_reasons(
    component_id: str, component: dict[str, Any], register: dict[str, Any]
) -> list[str]:
    """Lista vazia significa que o componente pode ser modelado."""
    solids = (component.get("cad") or {}).get("solids") or []
    if not solids:
        return ["nenhum sólido declarado em 'cad.solids'"]
    reasons: list[str] = []
    for solid in solids:
        missing = [field for field in SOLID_SIZE_KEYS if not solid.get(field)]
        if missing:
            reasons.append(f"sólido {solid.get('name')!r}: faltam as chaves {missing}")
    for key in required_dimension_keys(component):
        reasons.extend(dimension_problems(component, key, register))
    return reasons


def confirmed_length(
    component_id: str, component: dict[str, Any], key: str, register: dict[str, Any]
) -> ConfirmedLength:
    """Única porta de entrada de um número para a geometria."""
    problems = dimension_problems(component, key, register)
    if problems:
        raise EnvelopeRefused(component_id, problems)
    dimension = component["dimensions"][key]
    return ConfirmedLength(
        component_id=component_id,
        key=key,
        value=float(dimension["value"]),
        sources=tuple(_manufacturer_sources(dimension, register["sources"])),
        token=_ISSUED,
    )


def box_specs(
    component_id: str, component: dict[str, Any], register: dict[str, Any]
) -> list[BoxSpec]:
    """Especificação das caixas de um componente. Recusa o componente inteiro
    se qualquer dimensão usada por qualquer sólido não for CONFIRMADO."""
    reasons = refusal_reasons(component_id, component, register)
    if reasons:
        raise EnvelopeRefused(component_id, reasons)

    specs: list[BoxSpec] = []
    for solid in component["cad"]["solids"]:
        used: list[ConfirmedLength] = []

        def take(field: str) -> ConfirmedLength:
            length = confirmed_length(component_id, component, solid[field], register)
            used.append(length)
            return length

        size = [take(field).value for field in SOLID_SIZE_KEYS]
        center = [0.0, 0.0, size[2] / 2.0]
        if solid.get("y_flush_max_with"):
            reference = take("y_flush_max_with").value
            if size[1] > reference + BBOX_TOLERANCE_MM:
                raise EnvelopeRefused(
                    component_id,
                    [f"sólido {solid['name']!r}: y maior que a referência de alinhamento"],
                )
            center[1] = reference / 2.0 - size[1] / 2.0
        if solid.get("grow"):
            margin = take("grow").value
            size = [value + 2.0 * margin for value in size]
        specs.append(
            BoxSpec(
                component_id=component_id,
                name=solid["name"],
                size=(size[0], size[1], size[2]),
                center=(center[0], center[1], center[2]),
                dimensions_used=tuple(used),
                token=_ISSUED,
            )
        )
    return specs


def plan(register: dict[str, Any]) -> tuple[dict[str, list[BoxSpec]], dict[str, list[str]]]:
    """Separa os componentes em modeláveis e recusados."""
    accepted: dict[str, list[BoxSpec]] = {}
    refused: dict[str, list[str]] = {}
    for component_id, component in register["components"].items():
        try:
            accepted[component_id] = box_specs(component_id, component, register)
        except EnvelopeRefused as refusal:
            refused[component_id] = refusal.reasons
    return accepted, refused


# --------------------------------------------------------------------------
# Geometria e exportação (cadquery)
# --------------------------------------------------------------------------


def _require_cadquery():
    try:
        import cadquery
    except ImportError as error:  # pragma: no cover - depende do ambiente
        raise SystemExit(
            "cadquery não está instalado neste Python. Ver mechanical/README.md "
            f"(erro original: {error})"
        )
    return cadquery


def build_solid(spec: BoxSpec):
    """Constrói a caixa. Só aceita BoxSpec, que só nasce de dimensões confirmadas."""
    if not isinstance(spec, BoxSpec):
        raise TypeError("build_solid só aceita BoxSpec")
    lengths = spec.dimensions_used
    if (
        spec.token is not _ISSUED
        or len(lengths) < len(SOLID_SIZE_KEYS)
        or any(not isinstance(length, ConfirmedLength) or length.token is not _ISSUED for length in lengths)
    ):
        raise EnvelopeRefused(
            getattr(spec, "component_id", "?"),
            ["BoxSpec não emitido por box_specs(): dimensões sem verificação de status e fonte"],
        )
    cq = _require_cadquery()
    return (
        cq.Workplane("XY")
        .box(*spec.size, centered=(True, True, True))
        .translate(spec.center)
    )


def _step_string(text: str) -> str:
    return text.encode("ascii", "replace").decode("ascii").replace("'", " ")


def _normalise_step_header(text: str, spec: BoxSpec, register_date: str, tool: str) -> str:
    """Troca a descrição e a data do cabeçalho: inclui o aviso de licença e
    torna o arquivo reprodutível (a data passa a ser a do registro)."""
    description = ",\n  ".join(
        f"'{_step_string(line)}'"
        for line in (
            "d-BMO-body v1 - envelope parametrico",
            f"{spec.component_id} ({spec.name})",
            "Caixa envolvente, nao e peca detalhada",
            f"SPDX-License-Identifier: {SPDX_LICENSE}",
            f"SPDX-FileCopyrightText: {SPDX_COPYRIGHT}",
            "Gerado por mechanical/cad/generate_models.py",
            "Fonte dos valores: mechanical/cad/components.yaml",
        )
    )
    file_name = (
        f"FILE_NAME('{spec.file_stem}.step',\n  '{register_date}T00:00:00',"
        f"('d-BMO-body'),('d-BMO-body'),\n  '{_step_string(tool)}','generate_models.py','');"
    )
    text, count_description = re.subn(
        r"FILE_DESCRIPTION\(.*?\);",
        lambda _match: f"FILE_DESCRIPTION((\n  {description}),'2;1');",
        text,
        count=1,
        flags=re.S,
    )
    text, count_name = re.subn(
        r"FILE_NAME\(.*?\);", lambda _match: file_name, text, count=1, flags=re.S
    )
    if count_description != 1 or count_name != 1:
        raise RuntimeError("cabeçalho STEP inesperado: não foi possível normalizar")
    # O exportador numera o produto de topo e a ocorrência com um contador do
    # processo; fixar os dois deixa a saída igual em qualquer execução.
    text, count_product = re.subn(
        r"PRODUCT\('Open CASCADE STEP translator [^']*',\s*'Open CASCADE STEP translator [^']*'",
        "PRODUCT('dBMO_envelope','dBMO_envelope'",
        text,
    )
    text, count_occurrence = re.subn(
        r"NEXT_ASSEMBLY_USAGE_OCCURRENCE\('\d+',", "NEXT_ASSEMBLY_USAGE_OCCURRENCE('1',", text
    )
    if count_product != 1 or count_occurrence != 1:
        raise RuntimeError("estrutura STEP inesperada: produto de topo não encontrado")
    return text


def measure_step(path: Path | str) -> tuple[tuple[float, float], ...]:
    """Caixa envolvente exata de um STEP: ((xmin, xmax), (ymin, ymax), (zmin, zmax))."""
    cq = _require_cadquery()
    from OCP.Bnd import Bnd_Box
    from OCP.BRepBndLib import BRepBndLib

    shape = cq.importers.importStep(str(path)).val()
    box = Bnd_Box()
    box.SetGap(0.0)
    BRepBndLib.AddOptimal_s(shape.wrapped, box, False, False)
    xmin, ymin, zmin, xmax, ymax, zmax = box.Get()
    return ((xmin, xmax), (ymin, ymax), (zmin, zmax))


def export_step(spec: BoxSpec, out_dir: Path, register_date: str) -> Path:
    cq = _require_cadquery()
    solid = build_solid(spec)
    # Cores só para leitura no visualizador: prata industrial para o envelope,
    # vermelho de sinalização translúcido para zonas de exclusão.
    if "keepout" in spec.name:
        color = cq.Color(0.851, 0.137, 0.196, 0.35)
    else:
        color = cq.Color(0.682, 0.702, 0.722, 1.0)
    path = out_dir / f"{spec.file_stem}.step"
    cq.Assembly(solid, name=spec.file_stem, color=color).export(str(path))
    tool = f"cadquery {cq.__version__}"
    text = path.read_text(encoding="ascii")
    path.write_text(_normalise_step_header(text, spec, register_date, tool), encoding="ascii")

    measured = measure_step(path)
    for axis, (expected, found) in enumerate(zip(spec.bounds, measured)):
        if any(abs(a - b) > BBOX_TOLERANCE_MM for a, b in zip(expected, found)):
            path.unlink()
            raise RuntimeError(
                f"{path.name}: caixa envolvente relida difere da especificada no eixo "
                f"{'xyz'[axis]} ({found} em vez de {expected})"
            )
    return path


def _sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def generate(
    register_path: Path | str = DEFAULT_REGISTER,
    out_dir: Path | str = DEFAULT_OUT,
) -> dict[str, Any]:
    """Gera os STEP dos componentes modeláveis e apaga os STEP que deixaram de
    ter direito a existir. Devolve o manifesto."""
    register = load_register(register_path)
    problems = validate_register(register)
    if problems:
        raise RegisterError("registro incoerente:\n  - " + "\n  - ".join(problems))
    accepted, refused = plan(register)
    register_date = str(register.get("register_date", "1970-01-01"))

    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    generated = []
    written: set[str] = set()
    for component_id, specs in accepted.items():
        for spec in specs:
            path = export_step(spec, out_dir, register_date)
            written.add(path.name)
            generated.append(
                {
                    "component": component_id,
                    "solid": spec.name,
                    "file": path.name,
                    "size_mm": [round(value, 6) for value in spec.size],
                    "center_mm": [round(value, 6) for value in spec.center],
                    "local_frame": register["components"][component_id]["cad"].get("local_frame"),
                    "dimensions_used": {
                        length.key: {"value_mm": length.value, "sources": list(length.sources)}
                        for length in spec.dimensions_used
                    },
                    "sha256": _sha256(path),
                }
            )

    removed = []
    for stale in sorted(out_dir.glob("*.step")):
        if stale.name not in written:
            stale.unlink()
            removed.append(stale.name)

    cq = _require_cadquery()
    manifest = {
        "spdx_license": SPDX_LICENSE,
        "register": Path(register_path).name,
        "register_date": register_date,
        "generator": "mechanical/cad/generate_models.py",
        "cadquery_version": cq.__version__,
        "rule": "geometria só para componentes cujas dimensões usadas são todas CONFIRMADO",
        "generated": generated,
        "refused": [
            {"component": component_id, "reasons": reasons}
            for component_id, reasons in refused.items()
        ],
        "removed_stale_files": removed,
    }
    (out_dir / MANIFEST_NAME).write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    return manifest


# --------------------------------------------------------------------------
# Linha de comando
# --------------------------------------------------------------------------


def _print_plan(accepted: dict[str, list[BoxSpec]], refused: dict[str, list[str]]) -> None:
    print(f"Modeláveis ({len(accepted)}):")
    for component_id, specs in accepted.items():
        for spec in specs:
            size = " x ".join(f"{value:g}" for value in spec.size)
            print(f"  {spec.file_stem}.step  [{size} mm]")
    print(f"Recusados ({len(refused)}):")
    for component_id, reasons in refused.items():
        print(f"  {component_id}")
        for reason in reasons:
            print(f"      - {reason}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--register", type=Path, default=DEFAULT_REGISTER)
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT)
    parser.add_argument(
        "--check",
        action="store_true",
        help="valida o registro e mostra o plano, sem gerar arquivos",
    )
    args = parser.parse_args(argv)

    register = load_register(args.register)
    problems = validate_register(register)
    if problems:
        print("Registro incoerente:", file=sys.stderr)
        for problem in problems:
            print(f"  - {problem}", file=sys.stderr)
        return 2

    accepted, refused = plan(register)
    if args.check:
        _print_plan(accepted, refused)
        return 0

    manifest = generate(args.register, args.out)
    _print_plan(accepted, refused)
    print(f"Arquivos em {args.out}:")
    for entry in manifest["generated"]:
        print(f"  {entry['file']}  sha256={entry['sha256'][:16]}...")
    for name in manifest["removed_stale_files"]:
        print(f"  removido (sem direito a existir): {name}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except BrokenPipeError:
        # Saída cortada por um pipe (por exemplo "| head"): encerrar em silêncio.
        import os

        os.dup2(os.open(os.devnull, os.O_WRONLY), sys.stdout.fileno())
        sys.exit(0)
