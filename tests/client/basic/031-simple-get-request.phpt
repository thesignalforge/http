--TEST--
Client: sendRequest() simple GET request
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

// ARRANGE: Create client and simple GET request using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $statusCode = $response->getStatusCode();
    $hasContentType = $response->hasHeader('Content-Type');
    $body = (string)$response->getBody();
    $data = json_decode($body, true);

    // ASSERT: Verify response is valid
    var_dump($statusCode === 200);
    var_dump($hasContentType);
    var_dump(isset($data['id']) && $data['id'] === 1);
    echo "GET request successful\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
GET request successful
