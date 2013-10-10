#include <disk-mgr/dm_service.h>

int main(int argc, char **argv)
{
    int           min, max;
    fds::DmQuery &query = fds::DmQuery::dm_query();

    query.dm_iops(&min, &max);
    return 0;
}
