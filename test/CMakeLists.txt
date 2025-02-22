# Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

mark_as_advanced(
    BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
    gmock_build_tests gtest_build_samples gtest_build_tests
    gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
)

add_library(stdlib1 SHARED IMPORTED )
set_target_properties(stdlib1 PROPERTIES   IMPORTED_LOCATION "/home/yfxu/lib/libthrift.so.0.15.0")

add_library(stdlib SHARED IMPORTED )
set_target_properties(stdlib PROPERTIES   IMPORTED_LOCATION "/home/yfxu/lib/libstdc++.so.6")
set(mLIBS  stdlib stdlib1)
 

include_directories("/home/yfxu/hdf5-1.10.10/src" "/home/yfxu/hdf5-1.10.10/c++/src"  "/home/yfxu/hdf5-1.10.10/hl/src" "/home/yfxu/apache-arrow-13.0.0/cpp/src")
link_directories("/home/yfxu/lib" )
add_library(ft_test ../src/trader/risk/common/self_trade_risk.cpp
                    ../src/trader/risk/risk_rule.cpp)
target_include_directories(ft_test PUBLIC ../src)
target_link_libraries(ft_test PUBLIC ft::ft_header ft::utils fmt hdf5 hdf5_hl arrow parquet ${mLIBS})
# event event_pthreads

set_target_properties(gtest PROPERTIES FOLDER third_party)
set_target_properties(gtest_main PROPERTIES FOLDER third_party)
set_target_properties(gmock PROPERTIES FOLDER third_party)
set_target_properties(gmock_main PROPERTIES FOLDER third_party)

macro(package_add_test TESTNAME FILES LIBRARIES)
    # create an exectuable in which the tests will be stored
    add_executable(${TESTNAME} ${FILES})
    # link the Google test infrastructure, mocking library, and a default main fuction to
    # the test executable.  Remove g_test_main if writing your own main function.
    target_link_libraries(${TESTNAME} gtest gmock gtest_main ${LIBRARIES})
    # gtest_discover_tests replaces gtest_add_tests,
    # see https://cmake.org/cmake/help/v3.10/module/GoogleTest.html for more options to pass to it
    gtest_discover_tests(${TESTNAME}
        # set a working directory so your project root so that you can find test data via paths relative to the project root
        WORKING_DIRECTORY ${PROJECT_DIR}
        PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
    )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()

package_add_test(test_arrow test_arrow.cpp ft_test)
package_add_test(test_hdf test_hdf.cpp ft_test)
package_add_test(test_ring_buffer_pop test_ring_buffer_pop.cpp ft_test)
package_add_test(test_ring_buffer_push test_ring_buffer_push.cpp ft_test)
package_add_test(test_redis_get test_redis_get.cpp ft_test)
package_add_test(test_redis_set test_redis_set.cpp ft_test)
# package_add_test(test_redis_pub test_redis_pub.cpp ft_test)
# package_add_test(test_redis_sub test_redis_sub.cpp ft_test)
package_add_test(test_cereal test_cereal.cpp ft_test)
package_add_test(test_datetime test_datetime.cpp ft_test)
package_add_test(test_decimal_price test_decimal_price.cpp ft_test)
package_add_test(test_self_trade_risk test_self_trade_risk.cpp ft_test)
package_add_test(test_order_book test_order_book.cpp ft_test)
package_add_test(test_ring_buffer test_ring_buffer.cpp ft_test)
package_add_test(test_yijinjing test_yijinjing.cpp yijinjing ft_test)
package_add_test(test_trader_db test_trader_db.cpp ft::component)
package_add_test(test_position_calculator test_position_calculator.cpp ft::component)
package_add_test(test_networking test_networking.cpp ft::component)
package_add_test(test_yijinjing_read test_yijinjing_read.cpp yijinjing ft_test)
package_add_test(test_yijinjing_write test_yijinjing_write.cpp yijinjing ft_test)
#package_add_test(test_advanced_match_engine test_advanced_match_engine.cpp ft::component gateway ft_test)
