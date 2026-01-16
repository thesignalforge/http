--TEST--
Client: Response implements PSR-7 ResponseInterface
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
use Psr\Http\Message\ResponseInterface;

// ARRANGE: Create and send request using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);

    // ASSERT: Verify response implements PSR-7 ResponseInterface
    var_dump($response instanceof ResponseInterface);
    var_dump(method_exists($response, 'getStatusCode'));
    var_dump(method_exists($response, 'getHeaders'));
    var_dump(method_exists($response, 'getBody'));
    var_dump(method_exists($response, 'getProtocolVersion'));
    var_dump(method_exists($response, 'getReasonPhrase'));
    echo "Response implements PSR-7 ResponseInterface\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
Response implements PSR-7 ResponseInterface
