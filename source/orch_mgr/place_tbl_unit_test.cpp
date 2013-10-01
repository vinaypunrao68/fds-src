/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <iostream> // NOLINT(*)
#include <string>
#include <vector>

#include "include/fds_placement_table.h"

int main(int argc, char *argv[]) {
  std::string key("My key");
  std::string data("My data");

  fds::fds_placement_table *tab;
  tab = new fds::fds_placement_table(8, 4);

  std::cout << "tab " << *tab << std::endl;

  fds::FdsDlt *dlt_a;
  dlt_a = new fds::FdsDlt(16, 4);
  std::cout << "dlt a " << *dlt_a << std::endl;
  std::vector<fds::fds_nodeid_t> node_list_a;
  node_list_a.push_back(32);
  node_list_a.push_back(14);
  node_list_a.push_back(87);
  dlt_a->insert(4, node_list_a);

  fds::FdsDlt *dlt_b;
  dlt_b = new fds::FdsDlt(16, 4);
  std::cout << "dlt b " << *dlt_b << std::endl;
  std::vector<fds::fds_nodeid_t> node_list_b;
  node_list_b.push_back(32);
  node_list_b.push_back(14);
  node_list_b.push_back(87);
  dlt_b->insert(4, node_list_b);

  if (*dlt_a == *dlt_b) {
    std::cout << "They're equal " << std::endl;
  } else {
    std::cout << "They're NOT equal " << std::endl;
  }

  node_list_b.push_back(75);
  fds::Error err(fds::ERR_OK);
  err = dlt_b->insert(4, node_list_b);
  if (err != fds::ERR_OK) {
    std::cout << "Can't insert, dup" << std::endl;
  }

  err = dlt_b->insert(5, node_list_b);
  if (err != fds::ERR_OK) {
    std::cout << "Can't insert, dup" << std::endl;
  }

  if (*dlt_a == *dlt_b) {
    std::cout << "They're equal " << std::endl;
  } else {
    std::cout << "They're NOT equal " << std::endl;
  }

  *dlt_b = *dlt_a;
  if (*dlt_a == *dlt_b) {
    std::cout << "They're equal " << std::endl;
  } else {
    std::cout << "They're NOT equal " << std::endl;
  }

  for (fds_uint32_t i = 0; i < 10; i++) {
    // rec = new fds::Record(key);

    // std::cout << "Record size is " << rec->size() << std::endl;

    // delete rec;
  }

  delete dlt_a;
  delete dlt_b;
  delete tab;

  return 0;
}
