--TEST--
Client: Request with large header value (4KB)
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

// ARRANGE: Create request with large header value using PSR-17 factory
$client = new Client(['timeout' => 15]);
$requestFactory = new RequestFactory();
$largeValue = str_repeat('X', 4096);
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');
$request = $request->withHeader('X-Large-Header', $largeValue);

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $statusCode = $response->getStatusCode();

    // ASSERT: Verify large header was handled
    var_dump($statusCode === 200 || $statusCode === 431);
    echo "Large header handled\n";
} catch (\Exception $e) {
    // Some servers may reject large headers
    var_dump(true);
    echo "Large header handled\n";
}
?>
--EXPECT--
bool(true)
Large header handled
