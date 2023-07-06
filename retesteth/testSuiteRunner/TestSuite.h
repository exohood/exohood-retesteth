/*
    This file is part of cpp-ethereum.

    cpp-ethereum is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    cpp-ethereum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
 */
/** @file
 * A base class for test suites
 */

#pragma once
#include <libdataobj/DataObject.h>
#include <retesteth/testSuiteRunner/TestSuiteHelperFunctions.h>
#include <boost/filesystem/path.hpp>
#include <functional>

namespace test
{
extern std::string const c_fillerPostf;
extern std::string const c_copierPostf;
extern std::string const c_pythonPostf;

class TestSuite
{
protected:
    virtual bool legacyTestSuiteFlag() const { return false; }

private:
    // Execute Test.json file
    void executeFile(boost::filesystem::path const& _file) const;
    bool verifyFillers(std::string const& _testFolder,
        std::vector<boost::filesystem::path>& _outdated,
        std::vector<boost::filesystem::path>& _all) const;
    void checkFillerExistance(std::string const& _testFolder,
        std::vector<boost::filesystem::path>& _outdated,
        std::vector<boost::filesystem::path>& _all) const;

    struct BoostPath
    {
        BoostPath(boost::filesystem::path _path) : m_path(_path) {}
        boost::filesystem::path const& path() const { return m_path; }
        boost::filesystem::path parent_path() { return m_path.parent_path(); }

    private:
        boost::filesystem::path m_path;
    };

public:
    TestSuite() {}
    virtual ~TestSuite() {}

    struct TestSuiteOptions
    {
        TestSuiteOptions() : doFilling(false), allowInvalidBlocks(false), isLegacyTests(false), calculateRelativeSrcPath(true)
        {}
        bool doFilling;           // pass the filling flag to doTest function
        bool allowInvalidBlocks;  // allow and check malicious blocks
        bool isLegacyTests;       // running old generated tests
        bool calculateRelativeSrcPath;  // put relative path to the test path in a filled test info
    };

    // Structures so not to mistake the paths and prevent bugs
    struct TestPath : BoostPath
    {
        TestPath(boost::filesystem::path _path) : BoostPath(_path) {}
    };

    struct FillerPath : BoostPath
    {
        FillerPath(boost::filesystem::path _path) : BoostPath(_path) {}
    };

    struct AbsoluteFilledTestPath : BoostPath
    {
        AbsoluteFilledTestPath(boost::filesystem::path _path) : BoostPath(_path) {}
    };

    struct AbsoluteFillerPath : BoostPath
    {
        AbsoluteFillerPath(boost::filesystem::path _path) : BoostPath(_path) {}
    };

    // Main test executive function. should be declared for each test suite. it fills and runs the test .json file
    virtual dataobject::spDataObject doTests(dataobject::spDataObject&, TestSuiteOptions& _options) const = 0;

    // Execute all tests from suiteFolder()/_testFolder/*
    // This functions checks that tests in the repo are updated with /src/suiteFillerFolder()/*Filler tests
    // Then it looks for a Filler.json file in /src/suiteFillerFolder()/_testFolder and generates the actual tests
    // If src test file is named Copier it will just copy it from /src folder because such test is crafted and
    // it's RLP could not be generated by the client
    // If the src test does not end up with either Filler.json or Copier.json an exception occurs.
    void runAllTestsInFolder(std::string const& _testFolder) const;

    // Execute Filler.json or Copier.json test file in a given folder
    void executeTest(std::string const& _testFolder, boost::filesystem::path const& _jsonFileName) const;

    // Execute Test.json file
    void runTestWithoutFiller(boost::filesystem::path const& _file) const;

    // Return full path to folder for tests from _testFolder
    AbsoluteFillerPath getFullPathFiller(std::string const& _testFolder) const;

    // Structure  <suiteFolder>/<testFolder>/<test>.json
    AbsoluteFilledTestPath getFullPathFilled(std::string const& _testFolder) const;
    void setFillerPathAdd(std::string&& _path) const { m_fillerPathAdd = std::move(_path); }

    //
    static void runFunctionForAllClients(std::function<void()> _func);

protected:
    // A folder of the test suite. like "VMTests". should be implemented for each test suite.
    virtual TestPath suiteFolder() const = 0;

    // A folder of the test suite in src folder. like "VMTestsFiller". should be implemented for
    // each test suite.
    virtual FillerPath suiteFillerFolder() const = 0;

    mutable std::string m_fillerPathAdd;

private:
    void _executeTest(std::string const& _testFolder, boost::filesystem::path const& _jsonFileName) const;
    bool _fillTest(TestSuite::TestSuiteOptions& _opt, boost::filesystem::path const& _fillerTestFilePath,
        AbsoluteFilledTestPath const& _outputTestFilePath) const;
    void _runTest(AbsoluteFilledTestPath const& _filledTestPath) const;

    void _fillCopier(testsuite::TestFileData& _testData, boost::filesystem::path const& _fillerTestFilePath,
        AbsoluteFilledTestPath const& _outputTestFilePath) const;
    bool _fillJsonYml(testsuite::TestFileData& _testData, boost::filesystem::path const& _fillerTestFilePath,
        AbsoluteFilledTestPath const& _outputTestFilePath, TestSuite::TestSuiteOptions& _opt) const;
    bool _fillPython(testsuite::TestFileData& _testData, boost::filesystem::path const&, AbsoluteFilledTestPath const&,
        boost::filesystem::path const&) const;
};

}  // namespace test
