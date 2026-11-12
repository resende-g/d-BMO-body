#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Gabriel Almeida Resende
# SPDX-License-Identifier: CERN-OHL-P-2.0
"""Autoteste de generate_models.py. Roda com Python puro:

    python test_generate_models.py

Cobre a regra de recusa (só dimensões CONFIRMADO viram geometria), a
integridade do registro real, a sincronia do Markdown e, se o cadquery estiver
instalado, a geometria gerada. Sem cadquery, os testes de geometria são
contados como IGNORADOS e o resultado diz isso.
"""

from __future__ import annotations

import copy
import sys
import tempfile
import traceback
from pathlib import Path

import generate_models as gm
import render_register

try:
    import cadquery  # noqa: F401

    HAVE_CADQUERY = True
except ImportError:
    HAVE_CADQUERY = False

MANUFACTURER_SOURCE = {
    "origin": "fabricante",
    "kind": "desenho de teste",
    "title": "fonte sintética do autoteste",
    "url": "https://example.invalid/desenho.pdf",
    "accessed": "2026-11-12",
}
PROJECT_SOURCE = {"origin": "projeto", "kind": "anotação", "title": "README", "url": None}


def synthetic_register() -> dict:
    """Registro mínimo com um componente inteiramente confirmado."""

    def confirmed(value: float) -> dict:
        return {
            "label": "cota de teste",
            "value": value,
            "status": gm.STATUS_CONFIRMED,
            "source": ["FAB"],
        }

    return {
        "schema_version": 1,
        "register_date": "2026-11-12",
        "units": "mm",
        "sources": {"FAB": dict(MANUFACTURER_SOURCE), "PROJ": dict(PROJECT_SOURCE)},
        "components": {
            "ok_part": {
                "name": "peça de teste",
                "cad": {
                    "local_frame": "teste",
                    "solids": [{"name": "envelope", "x": "a", "y": "b", "z": "c"}],
                },
                "dimensions": {"a": confirmed(10.0), "b": confirmed(20.0), "c": confirmed(3.0)},
            }
        },
    }


def with_dimension(**changes) -> dict:
    """Registro sintético com a dimensão `c` de ok_part alterada."""
    register = synthetic_register()
    dimension = register["components"]["ok_part"]["dimensions"]["c"]
    for key, value in changes.items():
        if value is KeyError:
            dimension.pop(key, None)
        else:
            dimension[key] = value
    return register


def is_refused(register: dict, component_id: str = "ok_part") -> bool:
    accepted, refused = gm.plan(register)
    assert (component_id in accepted) != (component_id in refused)
    return component_id in refused


# --------------------------------------------------------------------------
# Regra de recusa (não precisa de cadquery)
# --------------------------------------------------------------------------


def test_baseline_synthetic_component_is_accepted():
    register = synthetic_register()
    assert gm.validate_register(register) == []
    assert not is_refused(register)
    (spec,) = gm.box_specs("ok_part", register["components"]["ok_part"], register)
    assert spec.size == (10.0, 20.0, 3.0)
    assert spec.center == (0.0, 0.0, 1.5)


def test_nominal_dimension_is_refused():
    register = with_dimension(status=gm.STATUS_NOMINAL, source=["PROJ"])
    assert gm.validate_register(register) == []
    assert is_refused(register)


def test_pending_dimension_is_refused():
    register = with_dimension(status=gm.STATUS_PENDING, value=None, source=[])
    assert gm.validate_register(register) == []
    assert is_refused(register)


def test_one_bad_dimension_refuses_the_whole_component():
    register = with_dimension(status=gm.STATUS_NOMINAL, source=["PROJ"])
    _, refused = gm.plan(register)
    assert refused["ok_part"] == ["c: status NOMINAL"]


def test_status_must_match_exactly():
    for status in ("confirmado", "CONFIRMADO ", "CONFIRMADO (fabricante)", "MEDIDO", None, True):
        assert is_refused(with_dimension(status=status)), status


def test_confirmed_without_value_is_refused():
    for value in (None, 0, -1.0, "3.0", True, float("nan"), float("inf")):
        assert is_refused(with_dimension(value=value)), value


def test_confirmed_without_manufacturer_url_is_refused():
    assert is_refused(with_dimension(source=[]))
    assert is_refused(with_dimension(source=["PROJ"]))
    assert is_refused(with_dimension(source=["NAO_EXISTE"]))
    register = synthetic_register()
    register["sources"]["FAB"]["url"] = None
    assert is_refused(register)
    register = synthetic_register()
    register["sources"]["FAB"]["url"] = "http://example.invalid/sem-tls.pdf"
    assert is_refused(register)
    register = synthetic_register()
    register["sources"]["FAB"]["origin"] = "secundaria"
    assert is_refused(register)
    register = synthetic_register()
    del register["sources"]["FAB"]["accessed"]
    assert is_refused(register)


def test_non_length_unit_is_refused():
    assert is_refused(with_dimension(unit="un"))
    assert is_refused(with_dimension(unit="grau"))


def test_missing_dimension_or_solid_is_refused():
    register = synthetic_register()
    del register["components"]["ok_part"]["dimensions"]["c"]
    assert is_refused(register)
    register = synthetic_register()
    register["components"]["ok_part"]["cad"]["solids"] = []
    assert is_refused(register)
    register = synthetic_register()
    del register["components"]["ok_part"]["cad"]
    assert is_refused(register)
    register = synthetic_register()
    del register["components"]["ok_part"]["cad"]["solids"][0]["z"]
    assert is_refused(register)


def test_optional_keys_also_need_confirmed_dimensions():
    register = synthetic_register()
    component = register["components"]["ok_part"]
    component["dimensions"]["margin"] = {
        "label": "folga",
        "value": 5.0,
        "status": gm.STATUS_NOMINAL,
        "source": ["PROJ"],
    }
    component["cad"]["solids"][0]["grow"] = "margin"
    assert is_refused(register)
    component["dimensions"]["margin"].update(status=gm.STATUS_CONFIRMED, source=["FAB"])
    assert not is_refused(register)
    (spec,) = gm.box_specs("ok_part", component, register)
    assert spec.size == (20.0, 30.0, 13.0)


def test_low_level_entry_points_refuse_directly():
    register = with_dimension(status=gm.STATUS_NOMINAL, source=["PROJ"])
    component = register["components"]["ok_part"]
    for call in (
        lambda: gm.confirmed_length("ok_part", component, "c", register),
        lambda: gm.box_specs("ok_part", component, register),
    ):
        try:
            call()
        except gm.EnvelopeRefused as refusal:
            assert refusal.reasons
        else:
            raise AssertionError("deveria ter recusado")
    try:
        gm.build_solid({"size": (1, 2, 3)})  # tipo errado: não passou pela verificação
    except TypeError:
        pass
    else:
        raise AssertionError("build_solid aceitou um objeto que não é BoxSpec")


def test_hand_made_specs_are_refused_by_the_builder():
    """Um BoxSpec montado à mão, com números quaisquer, não vira geometria."""
    fake_length = gm.ConfirmedLength("ok_part", "a", 10.0, ("FAB",))
    for lengths in ((), (fake_length, fake_length, fake_length)):
        spec = gm.BoxSpec("ok_part", "envelope", (10.0, 20.0, 3.0), (0.0, 0.0, 1.5), lengths)
        try:
            gm.build_solid(spec)
        except gm.EnvelopeRefused:
            pass
        else:
            raise AssertionError("build_solid aceitou BoxSpec que não veio de box_specs()")


def test_validator_flags_inconsistent_rows():
    assert gm.validate_register(with_dimension(status=gm.STATUS_PENDING)) != []
    assert gm.validate_register(with_dimension(status="MEDIDO")) != []
    assert gm.validate_register(with_dimension(source=["PROJ"])) != []
    assert gm.validate_register(with_dimension(status=gm.STATUS_NOMINAL, source=[])) != []


def test_only_confirmed_is_modelable():
    assert gm.MODELABLE_STATUSES == frozenset({"CONFIRMADO"})


# --------------------------------------------------------------------------
# Registro real
# --------------------------------------------------------------------------


def _independent_expectation(register: dict) -> set[str]:
    """Reimplementação deliberadamente simples da regra, para conferir plan()."""
    expected = set()
    for component_id, component in register["components"].items():
        keys = []
        for solid in component.get("cad", {}).get("solids", []):
            for field in ("x", "y", "z", "y_flush_max_with", "grow"):
                if solid.get(field):
                    keys.append(solid[field])
        if keys and all(
            component["dimensions"].get(key, {}).get("status") == "CONFIRMADO" for key in keys
        ):
            expected.add(component_id)
    return expected


def test_real_register_is_valid():
    register = gm.load_register()
    assert gm.validate_register(register) == []


def test_real_register_plan_follows_the_rule():
    register = gm.load_register()
    accepted, refused = gm.plan(register)
    assert set(accepted) == _independent_expectation(register)
    assert set(accepted) | set(refused) == set(register["components"])
    assert not set(accepted) & set(refused)
    for component_id, reasons in refused.items():
        assert reasons, component_id
    for component_id, specs in accepted.items():
        for spec in specs:
            for length in spec.dimensions_used:
                dimension = register["components"][component_id]["dimensions"][length.key]
                assert dimension["status"] == "CONFIRMADO"
                assert length.sources


def test_real_register_expected_models():
    register = gm.load_register()
    accepted, refused = gm.plan(register)
    assert set(accepted) == {"esp32_s3_wroom_1"}, set(accepted)
    sizes = {spec.name: spec for spec in accepted["esp32_s3_wroom_1"]}
    assert sizes["envelope"].size == (18.0, 25.5, 3.1)
    assert sizes["antenna_zone"].size == (18.0, 5.95, 3.1)
    assert abs(sizes["antenna_zone"].bounds[1][1] - 12.75) < 1e-9
    assert all(
        abs(found - expected) < 1e-9
        for found, expected in zip(sizes["antenna_keepout"].size, (48.0, 35.95, 33.1))
    )
    # Peças com valor só nominal ou pendente continuam recusadas.
    for component_id in ("display_msp3218", "battery_4070115", "esp32_s3_devkitc_1",
                         "card_edge_socket_dk920a_20m", "pn532_nfc_v3", "cc1101_module"):
        assert component_id in refused, component_id


def test_downgrading_a_real_dimension_removes_the_model():
    register = copy.deepcopy(gm.load_register())
    register["components"]["esp32_s3_wroom_1"]["dimensions"]["thickness"]["status"] = "NOMINAL"
    accepted, refused = gm.plan(register)
    assert "esp32_s3_wroom_1" in refused and not accepted


def test_markdown_register_is_in_sync():
    register = gm.load_register()
    markdown = render_register.DEFAULT_MARKDOWN.read_text(encoding="utf-8")
    assert render_register.current_block(markdown) == render_register.render(register), (
        "COMPONENT_DIMENSIONS.md defasado: rodar render_register.py"
    )


# --------------------------------------------------------------------------
# Geometria (precisa de cadquery)
# --------------------------------------------------------------------------


def _write_register(directory: Path, register: dict) -> Path:
    import yaml

    path = directory / "register.yaml"
    path.write_text(yaml.safe_dump(register, allow_unicode=True), encoding="utf-8")
    return path


def _extent(bounds) -> tuple[float, ...]:
    return tuple(round(high - low, 6) for low, high in bounds)


def geometry_test_writes_only_confirmed_components():
    register = synthetic_register()
    nominal = copy.deepcopy(register["components"]["ok_part"])
    nominal["dimensions"]["c"].update(status=gm.STATUS_NOMINAL, source=["PROJ"])
    pending = copy.deepcopy(register["components"]["ok_part"])
    pending["dimensions"]["a"].update(status=gm.STATUS_PENDING, value=None, source=[])
    register["components"]["nominal_part"] = nominal
    register["components"]["pending_part"] = pending
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        out = tmp_path / "out"
        out.mkdir()
        # Arquivo antigo de um componente que não tem mais direito a modelo.
        (out / "nominal_part__envelope.step").write_text("resto de execução anterior")
        manifest = gm.generate(_write_register(tmp_path, register), out)
        assert sorted(path.name for path in out.glob("*.step")) == ["ok_part__envelope.step"]
        assert manifest["removed_stale_files"] == ["nominal_part__envelope.step"]
        assert {entry["component"] for entry in manifest["refused"]} == {"nominal_part", "pending_part"}
        assert _extent(gm.measure_step(out / "ok_part__envelope.step")) == (10.0, 20.0, 3.0)
        header = (out / "ok_part__envelope.step").read_text(encoding="ascii")[:900]
        assert "SPDX-License-Identifier: CERN-OHL-P-2.0" in header
        assert (out / gm.MANIFEST_NAME).exists()


def geometry_test_real_register_outputs():
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp)
        manifest = gm.generate(gm.DEFAULT_REGISTER, out)
        names = sorted(path.name for path in out.glob("*.step"))
        assert names == [
            "esp32_s3_wroom_1__antenna_keepout.step",
            "esp32_s3_wroom_1__antenna_zone.step",
            "esp32_s3_wroom_1__envelope.step",
        ], names
        assert _extent(gm.measure_step(out / names[2])) == (18.0, 25.5, 3.1)
        assert _extent(gm.measure_step(out / names[1])) == (18.0, 5.95, 3.1)
        assert _extent(gm.measure_step(out / names[0])) == (48.0, 35.95, 33.1)
        zone = gm.measure_step(out / names[1])
        assert abs(zone[1][0] - 6.8) < 1e-6 and abs(zone[1][1] - 12.75) < 1e-6
        assert abs(zone[2][0]) < 1e-6
        # Segunda execução: mesmos bytes (saída reprodutível).
        first = {entry["file"]: entry["sha256"] for entry in manifest["generated"]}
        second = gm.generate(gm.DEFAULT_REGISTER, out)
        assert first == {entry["file"]: entry["sha256"] for entry in second["generated"]}


def geometry_test_committed_outputs_match_register():
    register = gm.load_register()
    accepted, _ = gm.plan(register)
    expected = {f"{spec.file_stem}.step": spec for specs in accepted.values() for spec in specs}
    committed = {path.name: path for path in gm.DEFAULT_OUT.glob("*.step")}
    assert set(committed) == set(expected), (sorted(committed), sorted(expected))
    for name, spec in expected.items():
        measured = gm.measure_step(committed[name])
        for (low, high), (found_low, found_high) in zip(spec.bounds, measured):
            assert abs(low - found_low) < 1e-6 and abs(high - found_high) < 1e-6, name


# --------------------------------------------------------------------------
# Execução
# --------------------------------------------------------------------------


def main() -> int:
    logic_tests = [
        value
        for name, value in sorted(globals().items())
        if name.startswith("test_") and callable(value)
    ]
    geometry_tests = [
        value
        for name, value in sorted(globals().items())
        if name.startswith("geometry_test_") and callable(value)
    ]
    passed = failed = skipped = 0
    for test in logic_tests + geometry_tests:
        if test in geometry_tests and not HAVE_CADQUERY:
            skipped += 1
            print(f"IGNORADO  {test.__name__} (cadquery ausente)")
            continue
        try:
            test()
        except Exception:  # noqa: BLE001 - o autoteste relata qualquer falha
            failed += 1
            print(f"FALHOU    {test.__name__}")
            traceback.print_exc()
        else:
            passed += 1
            print(f"ok        {test.__name__}")
    print(f"\n{passed} ok, {failed} falharam, {skipped} ignorados")
    if skipped:
        print("Atenção: a geometria NÃO foi verificada nesta execução (instalar cadquery).")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
