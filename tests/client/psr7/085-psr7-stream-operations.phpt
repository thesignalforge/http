--TEST--
Client: PSR-7 Stream operations on response body
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

// ARRANGE: Create and send request using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');

// ACT: Send the request and test stream operations
try {
    $response = $client->sendRequest($request);
    $stream = $response->getBody();

    // ASSERT: Verify PSR-7 Stream operations
    var_dump($stream->isReadable());
    var_dump($stream->isSeekable());
    var_dump($stream->getSize() > 0);

    $contents = $stream->getContents();
    var_dump(strlen($contents) > 0);

    echo "PSR-7 Stream operations work correctly\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
PSR-7 Stream operations work correctly
