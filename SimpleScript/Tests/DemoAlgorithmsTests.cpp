#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

TEST_CASE("algorithms: bubble sort", "[demo.algorithms]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, R"(
        fun bubbleSort(arr) {
            var n = length(arr);
            var i = 0;
            var j = 0;
            while (i < n - 1) {
                j = 0;
                while (j < n - i - 1) {
                    if (arr[j] > arr[j + 1]) {
                        var temp = copy(arr[j]);
                        arr[j] = copy(arr[j + 1]);
                        arr[j + 1] = temp;
                    }
                    j = j + 1;
                }
                i = i + 1;
            }
            return arr;
        }
        var data = list();
        pushback(data, 64);
        pushback(data, 34);
        pushback(data, 25);
        pushback(data, 12);
        pushback(data, 22);
        pushback(data, 11);
        pushback(data, 90);
        var sorted = bubbleSort(data);
        var r1 = sorted[0]; var r2 = sorted[1]; var r3 = sorted[2];
        var r4 = sorted[3]; var r5 = sorted[4]; var r6 = sorted[5]; var r7 = sorted[6];
        print(r1); print("-"); print(r2); print("-"); print(r3); print("-"); print(r4); print("-"); print(r5); print("-"); print(r6); print("-"); print(r7);
    )") == "11-12-22-25-34-64-90");
}

TEST_CASE("algorithms: binary search", "[demo.algorithms]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, R"(
        fun binarySearch(arr, target) {
            var left = 0;
            var right = length(arr) - 1;
            while (left <= right) {
                var mid = left + (right - left) / 2;
                if (arr[mid] == target) {
                    return mid;
                }
                if (arr[mid] < target) {
                    left = mid + 1;
                } else {
                    right = mid - 1;
                }
            }
            return -1;
        }
        var data = [2, 3, 4, 10, 40];
        var res1 = binarySearch(data, 10);
        var res2 = binarySearch(data, 5);
        print(res1);
        print("-");
        print(res2);
    )") == "3--1");
}

TEST_CASE("algorithms: fibonacci iterative", "[demo.algorithms]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, R"(
        fun fibonacci(n) {
            if (n <= 0) { return 0; }
            if (n == 1) { return 1; }
            var a = 0;
            var b = 1;
            var i = 2;
            for (i = 2; i <= n; i++) {
                var temp = a + b;
                a = b;
                b = temp;
            }
            return b;
        }
        var result = fibonacci(10);
        print(result);
    )") == "55");
}

TEST_CASE("algorithms: fizzbuzz", "[demo.algorithms]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, R"(
        fun fizzBuzz(n) {
            var result = [];
            var i = 1;
            while (i <= n) {
                if (i % 15 == 0) {
                    pushback(result, "FizzBuzz");
                } else if (i % 3 == 0) {
                    pushback(result, "Fizz");
                } else if (i % 5 == 0) {
                    pushback(result, "Buzz");
                } else {
                    pushback(result, i + 0);
                }
                i = i + 1;
            }
            return result;
        }
        var res = fizzBuzz(5);
        var r1 = res[0];
        var r2 = res[1];
        var r3 = res[2];
        var r4 = res[3];
        var r5 = res[4];
        print(r1); print("-"); print(r2); print("-"); print(r3); print("-"); print(r4); print("-"); print(r5);
    )") == "1-2-Fizz-4-Buzz");
}
