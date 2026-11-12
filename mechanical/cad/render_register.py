#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Gabriel Almeida Resende
# SPDX-License-Identifier: CERN-OHL-P-2.0
"""Escreve as tabelas do registro em docs/mechanical/COMPONENT_DIMENSIONS.md.

A fonte é components.yaml. O bloco entre os marcadores BEGIN/END do Markdown é
gerado; o resto do documento é escrito à mão e não é tocado.

Uso:
    python render_register.py          # atualiza o Markdown
    python render_register.py --check  # falha (código 1) se o Markdown estiver defasado

Só depende de PyYAML.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Any

from generate_models import (
    DEFAULT_REGISTER,
    STATUS_CONFIRMED,
    STATUS_NOMINAL,
    STATUS_PENDING,
    load_register,
    plan,
    validate_register,
)

HERE = Path(__file__).resolve().parent
DEFAULT_MARKDOWN = HERE.parent.parent / "docs" / "mechanical" / "COMPONENT_DIMENSIONS.md"

BEGIN = "<!-- BEGIN: REGISTRO-GERADO (não editar à mão; fonte: mechanical/cad/components.yaml) -->"
END = "<!-- END: REGISTRO-GERADO -->"

STATUS_LABEL = {
    STATUS_CONFIRMED: "CONFIRMADO (fabricante)",
    STATUS_NOMINAL: "NOMINAL (anúncio/designação)",
    STATUS_PENDING: "PENDENTE (medir com paquímetro)",
}


def format_number(value: Any, unit: str = "mm") -> str:
    """Número com vírgula decimal. Contagens saem como inteiro."""
    if value is None:
        return "—"
    if unit == "un":
        return str(int(value))
    text = f"{float(value):.4f}".rstrip("0")
    if text.endswith("."):
        text += "0"
    return text.replace(".", ",")


def cell(text: Any) -> str:
    if text is None or text == "":
        return "—"
    return str(text).replace("|", "\\|").replace("\n", " ")


def _source_cell(source_ids: list[str]) -> str:
    if not source_ids:
        return "—"
    return ", ".join(f"`{source_id}`" for source_id in source_ids)


def render(register: dict[str, Any]) -> str:
    accepted, refused = plan(register)
    components = register["components"]
    sources = register["sources"]
    lines: list[str] = [BEGIN, ""]

    # ---- Fontes ---------------------------------------------------------
    lines += [
        "## 2. Fontes",
        "",
        f"Data do registro: {register.get('register_date')}. Todas as fontes abaixo foram abertas nessa data.",
        "",
        "### 2.1 Fontes abertas",
        "",
        "| ID | Origem | Tipo | Documento e local | URL | SHA-256 do arquivo lido |",
        "|---|---|---|---|---|---|",
    ]
    for source_id, source in sources.items():
        url = f"<{source['url']}>" if source.get("url") else "—"
        sha = f"`{source['sha256']}`" if source.get("sha256") else "—"
        document = cell(source.get("title"))
        if source.get("location"):
            document += ". Local: " + cell(source["location"])
        lines.append(
            f"| `{source_id}` | {cell(source.get('origin'))} | {cell(source.get('kind'))} "
            f"| {document} | {url} | {sha} |"
        )
    lines += [
        "",
        "### 2.2 Documentos de fabricante que não puderam ser abertos",
        "",
        "Nenhum valor destes documentos foi usado. Eles são o caminho para promover linhas a `CONFIRMADO`.",
        "",
        "| Documento | URL | Motivo |",
        "|---|---|---|",
    ]
    for entry in register.get("unreachable_sources") or []:
        url = f"<{entry['url']}>" if entry.get("url") else "—"
        lines.append(f"| {cell(entry.get('what'))} | {url} | {cell(entry.get('reason'))} |")

    # ---- Resumo ---------------------------------------------------------
    lines += [
        "",
        "## 3. Resumo por componente",
        "",
        "\"Modelo CAD\" indica se `mechanical/cad/generate_models.py` gera envelope para o componente.",
        "",
        "| Componente | Peça | Qtd | CONFIRMADO | NOMINAL | PENDENTE | Modelo CAD |",
        "|---|---|---|---:|---:|---:|---|",
    ]
    totals = {STATUS_CONFIRMED: 0, STATUS_NOMINAL: 0, STATUS_PENDING: 0}
    for component_id, component in components.items():
        counts = {STATUS_CONFIRMED: 0, STATUS_NOMINAL: 0, STATUS_PENDING: 0}
        for dimension in component["dimensions"].values():
            counts[dimension["status"]] += 1
            totals[dimension["status"]] += 1
        if component_id in accepted:
            model = "sim: " + ", ".join(spec.name for spec in accepted[component_id])
        else:
            model = "não: " + "; ".join(refused[component_id])
        lines.append(
            f"| `{component_id}` | {cell(component.get('name'))} | {cell(component.get('qty'))} "
            f"| {counts[STATUS_CONFIRMED]} | {counts[STATUS_NOMINAL]} | {counts[STATUS_PENDING]} "
            f"| {cell(model)} |"
        )
    lines.append(
        f"| **Total** | {len(components)} componentes | | {totals[STATUS_CONFIRMED]} "
        f"| {totals[STATUS_NOMINAL]} | {totals[STATUS_PENDING]} "
        f"| {len(accepted)} com modelo, {len(refused)} recusados |"
    )

    # ---- Registro -------------------------------------------------------
    lines += ["", "## 4. Registro por componente", ""]
    default_unit = register.get("units", "mm")
    for component_id, component in components.items():
        lines += [
            f"### `{component_id}` — {component.get('name')}",
            "",
            f"{component.get('role')} Quantidade: {cell(component.get('qty'))}.",
            "",
            "| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |",
            "|---|---|---:|---|---|---|---|---|---|",
        ]
        for key, dimension in component["dimensions"].items():
            unit = dimension.get("unit", default_unit)
            tolerance = dimension.get("tolerance")
            if tolerance is None:
                tolerance = "não publicada" if dimension["status"] != STATUS_PENDING else "—"
            lines.append(
                f"| `{component_id}.{key}` | {cell(dimension.get('label'))} "
                f"| {format_number(dimension.get('value'), unit)} | {cell(unit)} "
                f"| {cell(tolerance)} | {STATUS_LABEL[dimension['status']]} "
                f"| {cell(dimension.get('basis'))} | {_source_cell(dimension.get('source') or [])} "
                f"| {cell(dimension.get('note'))} |"
            )
        lines.append("")

    # ---- Parâmetros do projeto -----------------------------------------
    lines += [
        "## 5. Parâmetros definidos pelo projeto",
        "",
        "Não são medidas de peça: são decisões do README. Não têm STATUS e não viram geometria no gerador.",
        "",
        "| ID | Parâmetro | Valor | Un. | Fonte | Observação |",
        "|---|---|---:|---|---|---|",
    ]
    for key, parameter in (register.get("design_parameters") or {}).items():
        unit = parameter.get("unit", default_unit)
        lines.append(
            f"| `{key}` | {cell(parameter.get('label'))} | {format_number(parameter.get('value'), unit)} "
            f"| {cell(unit)} | {cell(parameter.get('source'))} | {cell(parameter.get('note'))} |"
        )
    lines += ["", END]
    return "\n".join(lines)


def replace_block(markdown: str, block: str) -> str:
    start = markdown.find(BEGIN)
    end = markdown.find(END)
    if start < 0 or end < 0 or end < start:
        raise SystemExit("marcadores BEGIN/END não encontrados no Markdown")
    return markdown[:start] + block + markdown[end + len(END):]


def current_block(markdown: str) -> str:
    start = markdown.find(BEGIN)
    end = markdown.find(END)
    if start < 0 or end < 0:
        return ""
    return markdown[start : end + len(END)]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--register", type=Path, default=DEFAULT_REGISTER)
    parser.add_argument("--markdown", type=Path, default=DEFAULT_MARKDOWN)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)

    register = load_register(args.register)
    problems = validate_register(register)
    if problems:
        print("Registro incoerente:", file=sys.stderr)
        for problem in problems:
            print(f"  - {problem}", file=sys.stderr)
        return 2

    block = render(register)
    markdown = args.markdown.read_text(encoding="utf-8")
    if args.check:
        if current_block(markdown) != block:
            print(f"{args.markdown} está defasado em relação a {args.register}", file=sys.stderr)
            return 1
        print("Markdown em dia com o registro.")
        return 0
    args.markdown.write_text(replace_block(markdown, block), encoding="utf-8")
    print(f"Atualizado: {args.markdown}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
