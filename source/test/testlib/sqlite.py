##############################################################################
# author: Philippe Ribeiro                                                   #
# email: philippe@formationds                                                #
# file: ec2.py                                                               #
##############################################################################
import sqlite3

def get_aws_credentials():
    conn = sqlite3.connect('config.db')
    cursor = conn.cursor()
    t = ('fds_testing', )
    cursor.execute('SELECT access_key, secret_key FROM aws WHERE user=?', t)
    return cursor.fetchone()
