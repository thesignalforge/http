--TEST--
Client constructs with proxy option
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

// Arrange: Configure proxy
$options = ['proxy' => 'http://proxy.example.com:8080'];

// Act: Create client with proxy
$client = new Client($options);

// Assert: Client should be created successfully
var_dump($client instanceof Client);
?>
--EXPECT--
bool(true)
