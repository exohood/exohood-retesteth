#include <retesteth/configs/Options.h>
using namespace std;
using namespace dataobject;

namespace retesteth::options
{
genRewardsCfgEELS::genRewardsCfgEELS()
{
const string eels_correctMiningReward_config = R"({
    "//comment" : "State Tests does not calculate mining reward in post conditions, so when filling a blockchain test out of it, the mining reward must be set",
    "Frontier":          "5000000000000000000",
    "Homestead":         "5000000000000000000",
    "EIP150":            "5000000000000000000",
    "EIP158":            "5000000000000000000",
    "Byzantium":         "3000000000000000000",
    "Constantinople":    "2000000000000000000",
    "ConstantinopleFix": "2000000000000000000",
    "Istanbul":          "2000000000000000000",
    "Berlin" :           "2000000000000000000",
    "London" :           "2000000000000000000",
    "ArrowGlacier" :     "2000000000000000000",
    "GrayGlacier" :      "2000000000000000000",

    "//comment" : "Retesteth calculate rewards on behalf of the tool when filling state tests",
    "YOLOv1" :           "2000000000000000000",
    "YOLOv2" :           "2000000000000000000",
    "YOLOv3" :           "2000000000000000000",
    "Aleut"  :           "2000000000000000000",
    "Paris"  :           "0",
    "Merge"  :           "0",
    "Merged"  :          "0",
    "Shanghai"  :        "0",
    "Cancun" :           "0",
    "Prague" :           "0"
})";

    spDataObject obj;
    (*obj)["path"] = "eels/genesis/correctMiningReward.json";
    (*obj)["content"] = eels_correctMiningReward_config;
    map_configs.addArrayObject(obj);
}
}  // namespace retesteth::options
