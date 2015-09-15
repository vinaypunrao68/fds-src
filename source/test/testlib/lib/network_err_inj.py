"""
Library to inject network errors into the FDS system by using iptables to emulate network partitions and
interruptions.
"""

import iptc

class NetworkErrorInjector(object):
    """
    Class to inject errors into FDS by writing IP tables rules to emulate network partitions and interruptions.
    This class provides methods to create, manage, and remove errors.
    """
    def __init__(self):
        self.__injected_rules = []

    def __apply_single_rule(self, ip, port):
        """
        Adds a single rule to the INPUT chain
        :param ip: Destination IP address to block
        :param port: Destination port to block
        :return: None
        """

        rule = iptc.Rule()
        #rule.dst = ip
        rule.protocol = "tcp"
        m = rule.create_match("tcp")
        m.dport = port
        t = rule.create_target("DROP")

        chain = iptc.Chain(iptc.Table(iptc.Table.FILTER), "INPUT")
        chain.insert_rule(rule)

        # Make sure we're tracking them so we can clear them later
        self.__injected_rules.append(rule)

    def __remove_single_rule(self, rule):

        chain = iptc.Chain(iptc.Table(iptc.Table.FILTER), "INPUT")
        chain.delete_rule(rule)
        self.__injected_rules.remove(rule)

    def add_drop_rules(self, targets):
        """
        Causes packets bound for ip:port to be DROPPED. This emulates a network partition or network error.
        :param targets: List of (ip, port) pairs to represent nodes to partition
        :return: None
        """

        # Add each rule one at a time
        for (ip, port) in targets:
            # We'll need to clean up inputs here, for now assume anything with localhost in it is 127.0.0.1
            if 'localhost' in ip.lower():
                ip = '127.0.0.1'

            self.__apply_single_rule(ip, port)


    def clear_all_rules(self):
        """
        Clears any rules that were previously added, leaving previously exisiting rules in tact
        :return: None
        """

        # Create a copy of the list to iterate over so we don't get funky behavior for trying to iterate over a list
        # we're deleting from
        for rule in self.__injected_rules[:]:
            self.__remove_single_rule(rule)


    def flush_all_rules(self):
        """
        !! DANGER !!
        Flushes ALL rules, including any that may have been established prior to running this script
        :return: None
        """

        table = iptc.Table(iptc.Table.FILTER)
        chain = iptc.Chain(table, "INPUT")
        chain.flush()
        table.flush()

        self.__injected_rules = []
