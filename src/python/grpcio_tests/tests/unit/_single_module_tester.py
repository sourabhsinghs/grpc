# Copyright 2023 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from typing import Sequence

import unittest
import sys
import os
import pkgutil


class SingleLoader(object):
    def __init__(self, pattern: str):
        loader = unittest.TestLoader()
        self.suite = unittest.TestSuite()
        tests = []

        for importer, module_name, is_package in pkgutil.walk_packages([os.path.dirname(os.path.relpath(__file__))]):
            if pattern in module_name:
                module = importer.find_module(module_name).load_module(module_name)
                tests.append(loader.loadTestsFromModule(module))

        if len(tests) != 1:
            raise AssertionError("Expected only 1 test module. Found {}".format(tests))
        self.suite.addTest(tests[0])

    def loadTestsFromNames(self, names: Sequence[str], module: str = None) -> unittest.TestSuite:
        return self.suite

if __name__ == "__main__":

    if len(sys.argv) != 2:
        print(f"USAGE: {sys.argv[0]} TARGET_MODULE", file=sys.stderr)

    target_module = sys.argv[1]

    test_kwargs = {}
    test_kwargs["verbosity"] = 3

    loader = SingleLoader(target_module)
    runner = unittest.TextTestRunner(**test_kwargs)
    result = runner.run(loader.suite)
    
    if not result.wasSuccessful():
        sys.exit("Test failure.")