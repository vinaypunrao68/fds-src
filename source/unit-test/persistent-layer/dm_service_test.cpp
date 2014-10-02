#include <iostream>
#include <fds_assert.h>
#include <persistent-layer/dm_service.h>

using namespace std;

static void
query_test(void)
{
}

int main(int argc, char **argv)
{
    int           min, max;
    fds::DmQuery &query = fds::DmQuery::dm_query();

    query.dm_iops(&min, &max);
    query_test();

    return 0;
}
