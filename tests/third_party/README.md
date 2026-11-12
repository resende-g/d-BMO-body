# Código de terceiros usado só nos testes

| Pasta | Projeto | Versão | Licença | Origem |
|---|---|---|---|---|
| `unity/` | Unity Test Framework (ThrowTheSwitch) | v2.6.1 (commit `cbcd08fa7de711053a3deec6339ee89cad5d2697`) | MIT (`unity/LICENSE.txt`) | <https://github.com/ThrowTheSwitch/Unity>, arquivos `src/unity.c`, `src/unity.h`, `src/unity_internals.h` |

Os arquivos foram copiados sem alteração. Para atualizar, copiar os mesmos três arquivos
de uma tag mais nova e ajustar esta tabela.

O Unity não entra no firmware gravado no aparelho: só é compilado nos testes nativos
(`tests/`, via CMake) e nos testes de bancada (`tests/hardware/`, via PlatformIO, que
traz a própria cópia do Unity).
