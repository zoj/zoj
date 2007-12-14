/*
 * Copyright 2007 Xu, Chuan <xuchuan@gmail.com>
 *
 * This file is part of ZOJ.
 *
 * ZOJ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * ZOJ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ZOJ. if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __UNITTEST_H
#define __UNITTEST_H

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/ui/text/TestRunner.h>

class TestFixture : public CppUnit::TestFixture {
};

#define TEST(name) \
    class __ ## name ## Test: public CPPUNIT_NS::TestCase {\
        CPPUNIT_TEST_SUITE(__ ## name ## Test);\
        CPPUNIT_TEST(test);\
        CPPUNIT_TEST_SUITE_END();\
        public:\
            void test();\
    };\
    CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(__ ## name ## Test, "alltest");\
    void __ ## name ## Test::test()

#define TEST_F(name, method) \
    class __ ## name ## _ ## method : public name {\
        CPPUNIT_TEST_SUITE(__ ## name ## _ ## method);\
        CPPUNIT_TEST(test);\
        CPPUNIT_TEST_SUITE_END();\
        public:\
               void setUp() { name::setUp(); }\
               void tearDown() { name::tearDown(); }\
               void test();\
    };\
    CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(__ ## name ## _ ## method,\
                                          "alltest");\
    void __ ## name ## _ ## method::test()

#define ASSERT_EQUAL CPPUNIT_ASSERT_EQUAL
#define ASSERT CPPUNIT_ASSERT

int main() {
    CPPUNIT_NS::TextUi::TestRunner runner;
    CPPUNIT_NS::TestFactoryRegistry &registry =
        CPPUNIT_NS::TestFactoryRegistry::getRegistry("alltest");
    runner.addTest(registry.makeTest());
    runner.run();
    return 0;
}

#define TESTDIR "testdata"

#endif
