#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stf/stf.h>
#define SMB_IMPL
#include <smb/smb.h>

STF_TEST_CASE(smb, test)
{
    SMB_BENCHMARK_BEGIN(smb, really_long_description_right_here_better_believe_it, .total_runs = 10)
    sleep(1);
    SMB_BENCHMARK_END;

    STF_EXPECT(true, .failure_msg = "this can't fail");
}

int main(void)
{
    return STF_RUN_TESTS();
}
