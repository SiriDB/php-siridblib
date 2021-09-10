# php-siridblib
An C extension for PHP. Gives access to commands in PHP code to connect to SiriDB Server and query/insert.

In order to be compatible with >= php8, we have removed the usage of TSRMLS_C and TSRMLS_CC. This makes this lib no longer compatible with php5.

## Methods
- siridb_connect('ip', port, 'user', 'pass', 'db_name') -> returns connection to siridb
- siridb_close(conn) -> closes given connection
- siridb_query(con, "query") -> Returns results for given query
- siridb_insert(con, points) -> Inserts given points. Must be in format: array(array(ts, value), ...)


## Compiling

### MacOS and Linux

Make sure you have `libsiridb` and `libqpack` installed on your system.
Make sure you have `phpize` available as command
For macos users run `brew install autoconf` before continuing

Run:
```
phpize
./configure
make
sudo make install
```

Don't forget to enable the extension via the php.ini file

## Usage

When installed, you are able to using the following in your php code.
To connect to a SiriDB server use:
```

$siridb_con = siridb_connect('127.0.0.1', 9000, 'iris', 'siri', 'testdata_1');
if (!$siridb_con) {
    print("Cannot connect with siridb");
}
```

When you want to query something use:
```
$result = siridb_query($siridb_con, "select * from 'serie_name'");
echo $result; //json
```

When you want to insert points use:
```
$result = siridb_insert($siridb_con, array(
    "test_serie" => array(
        array(1582189879, 1.00003),
        array(1582189880, 1.00004)
    ),
    "test_serie2" => array(
        array(1582189879, 1.00003),
        array(1582189880, 1.00004)
    )
));
```

When you want to close the connection, use:
```
siridb_close($siridb_con);
```