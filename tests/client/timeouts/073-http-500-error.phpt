--TEST--
Client: HTTP 500 Internal Server Error response
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

// ARRANGE: Create request to endpoint returning 500 using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/http/500');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);

    // ASSERT: Verify 500 status is returned (not thrown as exception)
    var_dump($response->getStatusCode() === 500);
    echo "HTTP 500 handled successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
HTTP 500 handled successfully
