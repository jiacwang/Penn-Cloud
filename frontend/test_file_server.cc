#include "frontend_defs.h"
#include "quorum/quorum.h"
#include <string>

#define PASS_MSG "\033[92mPASS\033[0m"
#define FAIL_MSG "\033[91mFAIL\033[0m"


bool test_cd(FileServer *fs, string path, string oracle) {
	printf("cwd = %s\r\n", fs->cwd.c_str());
	printf("cd %s\r\n", path.c_str());
	fs->cd(path);
	return fs->cwd.compare(oracle) == 0;
}

int main(int argc, char const *argv[])
{
	Quorum q("quorum/databases.txt");
	FileServer fs(&q, "default");
	printf("%s\r\n", (test_cd(&fs, "/", "/") ? PASS_MSG : FAIL_MSG));
	printf("%s\r\n", (test_cd(&fs, "../", "/") ? PASS_MSG : FAIL_MSG));
	printf("%s\r\n", (test_cd(&fs, "./", "/") ? PASS_MSG : FAIL_MSG));
	printf("%s\r\n", (test_cd(&fs, "./hello", "/hello") ? PASS_MSG : FAIL_MSG));
	printf("%s\r\n", (test_cd(&fs, "../", "/") ? PASS_MSG : FAIL_MSG));
	printf("%s\r\n", (test_cd(&fs, "/hi", "/hi") ? PASS_MSG : FAIL_MSG));
	printf("%s\r\n", (test_cd(&fs, "./test/something", "/hi/test/something") ? PASS_MSG : FAIL_MSG));
	printf("%s\r\n", (test_cd(&fs, "./else", "/hi/test/something/else") ? PASS_MSG : FAIL_MSG));
	printf("%s\r\n", (test_cd(&fs, "../cool", "/hi/test/something/cool") ? PASS_MSG : FAIL_MSG));
	printf("%s\r\n", (test_cd(&fs, "../", "/hi/test/something") ? PASS_MSG : FAIL_MSG));
	printf("%s\r\n", (test_cd(&fs, "../", "/hi/test") ? PASS_MSG : FAIL_MSG));
	printf("%s\r\n", (test_cd(&fs, "../", "/hi") ? PASS_MSG : FAIL_MSG));
	printf("%s\r\n", (test_cd(&fs, "../", "/") ? PASS_MSG : FAIL_MSG));

	return 0;
}