#include <stdlib.h>
#include <string.h>
#include <stf/stf.h>
#define SMB_IMPL
#include <smb/smb.h>

STF_TEST_CASE(smb, test)
{
    SMB_BENCHMARK_BEGIN(ass, ass)
    SMB_BENCHMARK_END;

    STF_EXPECT(true, .failure_msg = "this can't fail");
}

int main(void)
{
    return STF_RUN_TESTS();
}
