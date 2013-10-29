#include <iostream>
#include <fds_assert.h>
#include <persistent_layer/dm_service.h>

using namespace std;

static void
query_test(void)
{
    fds::DmDiskInfo     *info;
    fds::DmDiskQuery     in;
    fds::DmDiskQueryOut  out;
    fds::DmQuery        &query = fds::DmQuery::dm_query();

    in.dmq_mask = fds::dmq_disk_info;
    query.dm_disk_query(in, &out);
    while (1) {
        info = out.query_pop();
        if (info != nullptr) {
            cout << "Max blks capacity: " << info->di_max_blks_cap << endl;
            cout << "Disk type........: " << info->di_disk_type << endl;
            cout << "Max iops.........: " << info->di_max_iops << endl;
            cout << "Min iops.........: " << info->di_min_iops << endl;
            cout << "Max latency (us).: " << info->di_max_latency << endl;
            cout << "Min latency (us).: " << info->di_min_latency << endl;
            delete info;
            continue;
        }
        break;
    }
}

int main(int argc, char **argv)
{
    int           min, max;
    fds::DmQuery &query = fds::DmQuery::dm_query();

    query.dm_iops(&min, &max);
    query_test();

    return 0;
}
