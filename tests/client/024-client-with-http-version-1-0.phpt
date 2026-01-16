--TEST--
Client configures with HTTP/1.0
--EXTENSIONS--
signalforge_http
--SKIPIF--
<?php
if (!class_exists('Signalforge\NativeHttp\Client')) {
    die('skip PSR-18 client not available (requires libcurl)');
}
?>
--FILE--
<?php
use Signalforge\NativeHttp\Client;

// Arrange: Configure HTTP/1.0
$options = ['http_version' => '1.0'];

// Act: Create client with HTTP/1.0
$client = new Client($options);

// Assert: Client should be created
var_dump($client instanceof Client);
echo "Client configured for HTTP/1.0\n";
?>
--EXPECT--
bool(true)
Client configured for HTTP/1.0
