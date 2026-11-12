#include <cstring>

#include "dbmo/storage/path.h"
#include "dbmo_test.h"

using dbmo::Status;
using namespace dbmo::storage;

void setUp() {}
void tearDown() {}

void test_valid_paths() {
  TEST_ASSERT_TRUE(isValidPath("/"));
  TEST_ASSERT_TRUE(isValidPath("/dbmo"));
  TEST_ASSERT_TRUE(isValidPath("/dbmo/config/config.txt"));
  TEST_ASSERT_TRUE(isValidPath("/dbmo/ir/tv_sala-2.ir"));
  TEST_ASSERT_TRUE(isValidPath(kConfigFile));
  TEST_ASSERT_TRUE(isValidPath(kLogFileOld));
}

void test_rejects_traversal_and_malformed_paths() {
  TEST_ASSERT_FALSE(isValidPath(nullptr));
  TEST_ASSERT_FALSE(isValidPath(""));
  TEST_ASSERT_FALSE(isValidPath("dbmo/config"));       // relativo
  TEST_ASSERT_FALSE(isValidPath("/dbmo/../segredo"));  // sobe de pasta
  TEST_ASSERT_FALSE(isValidPath("/dbmo/./config"));
  TEST_ASSERT_FALSE(isValidPath("/dbmo//config"));  // componente vazio
  TEST_ASSERT_FALSE(isValidPath("/dbmo/config/"));  // barra no fim
  TEST_ASSERT_FALSE(isValidPath("/dbmo/a b"));      // espaço
  TEST_ASSERT_FALSE(isValidPath("/dbmo/a\\b"));     // barra invertida
  TEST_ASSERT_FALSE(isValidPath("/dbmo/a:b"));
  TEST_ASSERT_FALSE(isValidPath("/dbmo/caf\xC3\xA9"));  // fora do ASCII permitido
  TEST_ASSERT_FALSE(isValidPath("/dbmo/a\nb"));
}

void test_rejects_oversized_paths_and_components() {
  char component[kMaxComponent + 3] = "/";
  memset(component + 1, 'a', kMaxComponent);
  TEST_ASSERT_TRUE(isValidPath(component));
  component[kMaxComponent + 1] = 'a';
  TEST_ASSERT_FALSE(isValidPath(component));

  char path[kMaxPath + 8] = {};
  size_t used = 0;
  while (used < kMaxPath) {  // "/aaaaaaa" repetido até passar do limite
    path[used++] = '/';
    for (int i = 0; i < 7 && used < sizeof(path) - 1; ++i) path[used++] = 'a';
  }
  TEST_ASSERT_FALSE(isValidPath(path));
}

void test_valid_names() {
  TEST_ASSERT_TRUE(isValidName("config.txt"));
  TEST_ASSERT_TRUE(isValidName(".oculto"));
  TEST_ASSERT_FALSE(isValidName(nullptr));
  TEST_ASSERT_FALSE(isValidName(""));
  TEST_ASSERT_FALSE(isValidName("."));
  TEST_ASSERT_FALSE(isValidName(".."));
  TEST_ASSERT_FALSE(isValidName("a/b"));
  TEST_ASSERT_FALSE(isValidName("../../etc"));
}

void test_join_path() {
  char out[kMaxPath];
  TEST_ASSERT_OK(joinPath(out, sizeof(out), kIrDir, "tv.ir"));
  TEST_ASSERT_EQUAL_STRING("/dbmo/ir/tv.ir", out);
  TEST_ASSERT_OK(joinPath(out, sizeof(out), "/", "dbmo"));
  TEST_ASSERT_EQUAL_STRING("/dbmo", out);

  TEST_ASSERT_STATUS(Status::InvalidArgument, joinPath(out, sizeof(out), kIrDir, "../config.txt"));
  TEST_ASSERT_STATUS(Status::InvalidArgument, joinPath(out, sizeof(out), "ir", "tv.ir"));
  TEST_ASSERT_STATUS(Status::InvalidArgument, joinPath(nullptr, 0, kIrDir, "tv.ir"));
  char small[8];
  TEST_ASSERT_STATUS(Status::NoSpace, joinPath(small, sizeof(small), kIrDir, "tv.ir"));
}

void test_cartridge_data_dir() {
  char out[kMaxPath];
  TEST_ASSERT_OK(cartridgeDataDir(0xFFFF, 0x0001, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("/dbmo/cartridges/ffff-0001", out);
  TEST_ASSERT_TRUE(isValidPath(out));
  char small[10];
  TEST_ASSERT_STATUS(Status::NoSpace, cartridgeDataDir(1, 2, small, sizeof(small)));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_valid_paths);
  RUN_TEST(test_rejects_traversal_and_malformed_paths);
  RUN_TEST(test_rejects_oversized_paths_and_components);
  RUN_TEST(test_valid_names);
  RUN_TEST(test_join_path);
  RUN_TEST(test_cartridge_data_dir);
  return UNITY_END();
}
