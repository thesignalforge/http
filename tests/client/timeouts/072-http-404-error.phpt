--TEST--
Client: HTTP 404 Not Found response
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

// ARRANGE: Create request to non-existent endpoint using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/http/404');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);

    // ASSERT: Verify 404 status is returned (not thrown as exception)
    var_dump($response->getStatusCode() === 404);
    echo "HTTP 404 handled successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
HTTP 404 handled successfully
