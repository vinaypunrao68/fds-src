import logging
try:
    import MySQLdb
except ImportError:
    import sys
    print("Error: The package python-mysqldb is not installed.  Unable "
          "to continue.")
    sys.exit(1)

class dbaccess:
    """ This class provides wrappers around database access commands from
    MySQLdb.
    """
    def __init__(self, host=None, user=None, password=None,
                 database_name=None, port=3306):
        self.log = logging.getLogger("mysqldb.dbaccess")
        self.log.setLevel(logging.INFO)
        self.db = None  # This is the MySQL connection object.
        self.host = host
        self.user = user
        self.password = password
        self.database_name = database_name
        self.port = port

    def connect_db(self):
        """ Connect to the database using the variables defined in __init__.
        The resulting database object will be stored in the self.db variable.
        """
        try:
            self.db = MySQLdb.connect(host=self.host,
                                      user=self.user,
                                      passwd=self.password,
                                      db=self.database_name,
                                      port=self.port)
        except MySQLdb.Error, err:
            self.log.error("Error %d: %s" %(err.args[0], err.args[1]))
            raise
        return

    def disconnect_db(self):
        """ Disconnect from the database if there is an active connection. """
        if self.db != None:
            try:
                self.db.close()
            except MySQLdb.Error, err:
                self.log.error("Error %d: %s" %(err.args[0], err.args[1]))
        return

    def query(self, sql_string):
        """ The function will grab a database cursor, send a sql_string with 
        the cursor close the cursor so it will sync with the database, and try 
        to catch any database error codes.  The return values are list of rows
        with dictionaries for the column names as keys.
        """
        if self.db == None:
            return ()
        
        rows = None
        try:
            self.log.debug(r"Sending SQL query string: %s" %sql_string)
            cursor = self.db.cursor(MySQLdb.cursors.DictCursor)
            cursor.execute(sql_string)
            rows = cursor.fetchall()
            cursor.close()
        except MySQLdb.Error, err:
            self.log.error("SQL query failed with code %d: %s"
                           %(err.args[0], err.args[1]))
            return ()
        return rows

    def update(self, sql_string):
        """ This function will grab a database cursor, send an update message
        with the sql_string, and close the cursor.  The return value is the 
        number of rows updated with the sql_string.  This function doesn't 
        limit the sql_string to update.
        """
        if self.db == None:
            return 0
        
        row_count = 0
        try:
            self.log.debug(r"Sending SQL update string: %s" %sql_string)
            cursor = self.db.cursor(MySQLdb.cursors.DictCursor)
            cursor.execute(sql_string)
            row_count = cursor.rowcount
            cursor.close()
        except MySQLdb.Error, err:
            self.log.error("SQL update failed with code %d: %s"
                           %(err.args[0], err.args[1]))
            return 0
        return row_count
