# php-siridblib
An C extension for PHP. Gives access to commands in PHP code to connect to SiriDB Server and query/insert.

## Methods
- siridb_connect('ip', port, 'user', 'pass', 'db_name') -> returns connection to siridb
- siridb_close(conn) -> closes given connection
- siridb_query(con, "query") -> Returns results for given query
- siridb_insert(con, points) -> Inserts given points. Must be in format: array(array(ts, value), ...)


## Compiling

### MacOS
Run:
`make && sudo make install`