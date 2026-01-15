--TEST--
Client: Response headers are properly parsed
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

// ARRANGE: Create a request using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');

// ACT: Send the request and examine response headers
try {
    $response = $client->sendRequest($request);
    $headers = $response->getHeaders();

    // ASSERT: Verify response headers are accessible
    var_dump($response->getStatusCode() === 200);
    var_dump(is_array($headers));
    var_dump(count($headers) > 0);
    var_dump($response->hasHeader('Content-Type'));
    echo "Response headers parsed successfully\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
Response headers parsed successfully
