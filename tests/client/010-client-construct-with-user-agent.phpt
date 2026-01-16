--TEST--
Client constructs with custom user agent
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

// Arrange: Configure custom user agent
$options = ['user_agent' => 'MyApp/1.0'];

// Act: Create client with custom user agent
$client = new Client($options);

// Assert: Client should be created successfully
var_dump($client instanceof Client);
?>
--EXPECT--
bool(true)
