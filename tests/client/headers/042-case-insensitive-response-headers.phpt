--TEST--
Client: Response headers are case-insensitive
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
use Signalforge\NativeHttp\RequestFactory;

// ARRANGE: Create a simple request using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');

// ACT: Send the request and check headers case-insensitively
try {
    $response = $client->sendRequest($request);

    // ASSERT: Verify case-insensitive header access
    var_dump($response->hasHeader('content-type'));
    var_dump($response->hasHeader('Content-Type'));
    var_dump($response->hasHeader('CONTENT-TYPE'));
    var_dump($response->hasHeader('CoNtEnT-TyPe'));
    echo "Case-insensitive header access works\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
Case-insensitive header access works
