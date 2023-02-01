#include "Transaction.h"
#include <Options.h>
#include <EthChecks.h>
#include <libdevcrypto/Common.h>
#include <retesteth/testStructures/Common.h>

using namespace test;
using namespace test::debug;
namespace test::teststruct
{

void Transaction::setChainID(VALUE const& _chainID) {
    if (m_secretKey.getCContent() != 0)
    {
        m_chainID = spVALUE(_chainID.copy());
        buildVRS();
    }
    else
        ETH_DC_MESSAGE(DC::LOWLOG, "Calling Transaction::setChainID for transaction without secretKey!");
}

Transaction::Transaction()
{
    m_chainID = spVALUE(new VALUE(Options::get().getCurrentConfig().cfgFile().defaultChainID()));
}

FH20 const& Transaction::sender() const
{
    if (m_sender.isEmpty())
    {
        if (m_secretKey.getCContent() != 0)
        {
            m_sender = convertSecretToPublic(m_secretKey);
            return m_sender;
        }
        else
        {
            try
            {
                dev::byte const v(m_v->asBigInt());
                dev::h256 const r(m_r->asString());
                dev::h256 const s(m_s->asString());
                dev::h256 const recoverHash = buildVRSHash();
                dev::SignatureStruct const sig(r,s,v);
                dev::Public const pubkey = dev::recover(sig, recoverHash);
                auto const address = dev::toAddress(pubkey);
                m_sender = spFH20(new FH20(dev::toHexPrefixed(address)));
                return m_sender;
            }
            catch (std::exception const& _ex)
            {
                ETH_WARNING("Transaction::sender() const:: error recovering sender! \n" + asDataObject()->asJson());
                m_sender = spFH20(new FH20(FH20::zero()));
                return m_sender;
            }
        }
    }
    else
        return m_sender;
}

}
