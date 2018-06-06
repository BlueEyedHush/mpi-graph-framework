
#include <gtest/gtest.h>
#include <utils/Config.h>

TEST(ConfigLoader, CorrectlyParsesEmptyArgList) {
	const char* argv[] = {"script"};
	auto cm = parseCli(1, argv);
	ASSERT_EQ(cm.size(), 0);
}

TEST(ConfigLoader, CorrectlyParsesSingleArg) {
	const char* argv[] = {"script", "-n", "val"};
	auto cm = parseCli(1, argv);
	ASSERT_EQ(cm.size(), 1);
	ASSERT_EQ(cm["n"], "val");
}

TEST(ConfigLoader, CorrectlyParsesTwoArgs) {
	const char* argv[] = {"script", "-n", "val", "-a" "value"};
	auto cm = parseCli(1, argv);
	ASSERT_EQ(cm.size(), 2);
	ASSERT_EQ(cm["n"], "val");
	ASSERT_EQ(cm["a"], "value");
}

TEST(ConfigLoader, FailsForEvenArgCount) {
	const char* argv[] = {"script", "-n"};
	ASSERT_ANY_THROW(parseCli(1, argv));
}

TEST(ConfigLoader, FailsForIncorrectlyFormattedOption) {
	const char* argv[] = {"script", "n", "val"};
	ASSERT_ANY_THROW(parseCli(1, argv));
}