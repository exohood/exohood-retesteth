#include "BlockchainTest.h"
#include <retesteth/EthChecks.h>
#include <retesteth/TestOutputHelper.h>
#include <retesteth/testStructures/Common.h>
#include <boost/test/framework.hpp>

using namespace std;
using namespace test::teststruct;

namespace
{
BlockchainTestEnv* readBlockchainTestEnv(DataObject const& _data)
{
    if (_data.count("baseFeePerGas"))
    {
        spDataObject diff = _data.atKey("difficulty").copy();
        (*diff).performModifier(mod_valueToCompactEvenHexPrefixed);
        if (VALUE(diff->asString()) != 0)
            return new BlockchainTestEnv1559(_data);
        else
        {
            if (_data.count("withdrawalsRoot"))
                return new BlockchainTestEnvShanghai(_data);
            else
                return new BlockchainTestEnvMerge(_data);
        }
        return new BlockchainTestEnv1559(_data);
    }
    return new BlockchainTestEnvLegacy(_data);
}
}  // namespace

namespace test::teststruct
{
BlockchainTestInFilled::BlockchainTestInFilled(spDataObject& _data)
{
    try
    {
        REQUIRE_JSONFIELDS(_data, "BlockchainTestInFilled " + _data->getKey(),
            {{"_info", {{DataType::Object}, jsonField::Required}},
                {"sealEngine", {{DataType::String}, jsonField::Optional}},
                {"genesisBlockHeader", {{DataType::Object}, jsonField::Required}},
                {"postState", {{DataType::Object}, jsonField::Optional}},
                {"postStateHash", {{DataType::String}, jsonField::Optional}},
                {"exceptions", {{DataType::Array}, jsonField::Optional}},
                {"genesisRLP", {{DataType::String}, jsonField::Required}},
                {"lastblockhash", {{DataType::String}, jsonField::Required}},
                {"network", {{DataType::String}, jsonField::Required}},
                {"pre", {{DataType::Object}, jsonField::Required}},
                {"blocks", {{DataType::Array}, jsonField::Required}}});

        m_name = _data->getKey();
        m_info = GCP_SPointer<Info>(new Info(_data->atKey("_info")));
        m_env = spBlockchainTestEnv(readBlockchainTestEnv(_data->atKey("genesisBlockHeader")));

        m_genesisRLP = spBYTES(new BYTES(_data->atKey("genesisRLP")));
        m_pre = spState(new State(MOVE(_data, "pre")));
        m_fork = spFORK(new FORK(_data->atKey("network")));
        m_sealEngine = SealEngine::NoProof;
        if (_data->count("sealEngine") && _data->atKey("sealEngine").asString() == sealEngineToStr(SealEngine::Ethash))
            m_sealEngine = SealEngine::Ethash;

        if (_data->count("postState"))
            m_post = spState(new State(MOVE(_data, "postState")));
        else
            m_postHash = spFH32(new FH32(_data->atKey("postStateHash")));

        string const c_blocks = "blocks";
        m_blocks.reserve(_data->atKey(c_blocks).getSubObjects().size());
        for (auto& el : _data.getContent().atKeyUnsafe(c_blocks).getSubObjectsUnsafe())
        {
            BlockchainTestBlock const block(el);
            m_blocks.push_back(std::move(block));
        }

        m_lastBlockHash = spFH32(new FH32(_data->atKey("lastblockhash")));

        if (_data->count("exceptions"))
        {
            for (size_t i = _data->atKey("exceptions").getSubObjects().size(); i > 0; i--)
                m_exceptions.push_back(_data->atKey("exceptions").getSubObjects().at(i - 1)->asString());
        }
    }
    catch (std::exception const& _ex)
    {
        ETH_ERROR_MESSAGE(string("BlockchainTestInFilled convertion error: ") + _ex.what());
    }
}

BlockchainTest::BlockchainTest(spDataObject& _data)
{
    try
    {
        ETH_ERROR_REQUIRE_MESSAGE(_data->type() == DataType::Object,
            TestOutputHelper::get().get().testFile().string() + " A test file must contain an object value (json/yaml).");
        ETH_ERROR_REQUIRE_MESSAGE(_data->getSubObjects().size() >= 1,
            TestOutputHelper::get().get().testFile().string() + " A test file must contain at least one test!");

        m_tests.reserve(_data->getSubObjects().size());
        for (auto& el : (*_data).getSubObjectsUnsafe())
        {
            TestOutputHelper::get().setCurrentTestInfo(TestInfo("BlockchainTest", el->getKey()));
            BlockchainTestInFilled const test(el);
            m_tests.push_back(std::move(test));
        }
    }
    catch (DataObjectException const& _ex)
    {
        ETH_ERROR_MESSAGE(_ex.what());
    }
}

void BlockchainTest::registerAllVectors() const
{
    string execTotal;
    auto const& helper = TestOutputHelper::get();
    string const suite = boost::unit_test::framework::current_test_case().full_name();
    string const execPrefix = string("-t ") + suite + " --";
    for (auto const& test : m_tests)
    {
        auto const filename = helper.testFile().stem().string();
        const string exec = string(" --singletest ") + filename + "/" + test.testName() + "\n";
        execTotal += execPrefix + exec;
    }
    TestOutputHelper::get().addTestVector(std::move(execTotal));
}

}  // namespace teststruct
