#include "BlockchainTestLogic.h"
#include "fillers/BlockchainTestFillerLogic.h"
#include <retesteth/EthChecks.h>
#include <retesteth/RPCSession.h>
#include <retesteth/testSuites/Common.h>


namespace test
{

/// Read and execute the test from the file
void RunTest(DataObject const& _testObject, TestSuite::TestSuiteOptions const& _opt)
{
    {
        TestInfo errorInfo("RunTestInit");
        TestOutputHelper::get().setCurrentTestInfo(errorInfo);
    }
    if (Options::get().logVerbosity > 1)
        std::cout << "Running " << TestOutputHelper::get().testName() << std::endl;
    scheme_blockchainTest inputTest(_testObject, _opt.isLegacyTests);
    RPCSession& session = RPCSession::instance(TestOutputHelper::getThreadID());

    // Info for genesis
    TestInfo errorInfo (inputTest.getNetwork(), 0);
    TestOutputHelper::get().setCurrentTestInfo(errorInfo);
    session.test_setChainParams(
        inputTest.getGenesisForRPC(inputTest.getNetwork(), inputTest.getEngine()).asJson());

    // for all blocks
    size_t blockNumber = 0;
    for (auto const& bdata : inputTest.getBlocks())
    {
        TestInfo errorInfo(inputTest.getNetwork(), blockNumber++);
        TestOutputHelper::get().setCurrentTestInfo(errorInfo);
        string const blHash = session.test_importRawBlock(bdata.atKey("rlp").asString());
        if (session.getLastRPCError().type() != DataType::Null)
        {
            if (!_opt.allowInvalidBlocks)
                ETH_ERROR_MESSAGE("Running blockchain test: " + session.getLastRPCError().atKey("message").asString());
        }

        // Check imported block against the fields in test
        if (bdata.count("blockHeader"))
        {
            // Check Blockheader
            test::scheme_block latestBlock = session.eth_getBlockByHash(blHash, true);
            string message = "Client return HEADER: " + latestBlock.getBlockHeader().asJson() +
                             "\n vs \n" + "Test HEADER: " + bdata.atKey("blockHeader").asJson();
            ETH_ERROR_REQUIRE_MESSAGE(latestBlock.getBlockHeader() == bdata.atKey("blockHeader"),
                "Client report different blockheader after importing the rlp than expected by "
                "test! \n" +
                    message);

            // Check Transaction count
            message =
                "Client return TRANSACTIONS: " + to_string(latestBlock.getTransactionCount()) +
                " vs " + "Test TRANSACTIONS: " +
                to_string(bdata.atKey("transactions").getSubObjects().size());
            ETH_ERROR_REQUIRE_MESSAGE(latestBlock.getTransactionCount() ==
                                          bdata.atKey("transactions").getSubObjects().size(),
                "Client report different transaction count after importing the rlp than expected "
                "by test! \n" +
                    message);

            // Verify transactions to one described in the fields
            for (auto const& tr : latestBlock.getTransactions())
            {
                ETH_ERROR_REQUIRE_MESSAGE(tr.atKey("blockHash") == blHash,
                    "Error checking remote transaction, tr blockHash is different to requested "
                    "block!");
                std::cerr << latestBlock.getTransactions().asJson() << std::endl;
                std::cerr << bdata.atKey("transactions").asJson() << std::endl;
            }

            // Check uncles count
            message =
                "Client return UNCLES: " + to_string(latestBlock.getUncleCount()) + " vs " +
                "Test UNCLES: " + to_string(bdata.atKey("uncleHeaders").getSubObjects().size());
            ETH_ERROR_REQUIRE_MESSAGE(
                latestBlock.getUncleCount() == bdata.atKey("uncleHeaders").getSubObjects().size(),
                "Client report different uncle count after importing the rlp than expected by "
                "test! \n" +
                    message);
        }
    }

    // wait for blocks to process
    // std::this_thread::sleep_for(std::chrono::seconds(10));

    scheme_block latestBlock = session.eth_getBlockByNumber(session.eth_blockNumber(), false);
    if (inputTest.getPost().isHash())
        validatePostHash(session, inputTest.getPost().getHash(), latestBlock);
    else
        compareStates(scheme_expectState(inputTest.getPost().getData()), session, latestBlock);

    if (inputTest.getLastBlockHash() != latestBlock.getBlockHash())
        ETH_ERROR_MESSAGE("lastblockhash does not match! remote: " + latestBlock.getBlockHash() +
                          ", test: " + inputTest.getLastBlockHash());

    if (inputTest.getData().count("genesisRLP"))
    {
        string const& genesisRLP = inputTest.getData().atKey("genesisRLP").asString();
        latestBlock = session.eth_getBlockByNumber(BlockNumber("0"), false);
        if (latestBlock.getBlockRLP() != genesisRLP)
            ETH_ERROR_MESSAGE("genesisRLP in test != genesisRLP on remote client! (" + genesisRLP +
                              "' != '" + latestBlock.getBlockRLP() + "'");
    }
}

DataObject DoTests(DataObject const& _input, TestSuite::TestSuiteOptions& _opt)
{
    checkDataObject(_input);
    checkAtLeastOneTest(_input);

    DataObject tests;
    // A blockchain test file contains many tests in one .json file
    for (auto const& i : _input.getSubObjects())
    {
        string const& testname = i.getKey();
        TestOutputHelper::get().setCurrentTestName(testname);

        if (_opt.doFilling)
        {
            {
                TestInfo errorInfo("TestFillerInit");
                TestOutputHelper::get().setCurrentTestInfo(errorInfo);
            }
            scheme_blockchainTestFiller testFiller(i);
            // Create a blockchain test for each network described in expect section
            for (auto& network : testFiller.getExpectSection().getAllNetworksFromExpectSection())
            {
                // select expect section corresponding to the network (if any)
                for (auto const& expect : testFiller.getExpectSection().getExpectSections())
                {
                    if (expect.getNetworks().count(network))
                    {
                        string const newtestname = testname + "_" + network;
                        TestOutputHelper::get().setCurrentTestName(newtestname);

                        DataObject testOutput;
                        FillTest(testFiller, network, _opt, testOutput);
                        if (testFiller.getData().count("_info"))
                            testOutput["_info"] = testFiller.getData().atKey("_info");
                        tests[newtestname] = testOutput;
                    }
                }
            }
        }
        else
        {
            // Select test by name if --singletest and --singlenet is set
            if (Options::get().singleTest)
            {
                if (!Options::get().singleSubTestName.empty() &&
                    testname != Options::get().singleSubTestName)
                    continue;
            }

            if (!Options::get().singleTestNet.empty())
            {
                if (i.count("network") &&
                    i.atKey("network").asString() != Options::get().singleTestNet)
                    continue;
            }

            RunTest(i, _opt);
        }
    }
    TestOutputHelper::get().registerTestRunSuccess();
    return tests;
}
}  // namespace test
