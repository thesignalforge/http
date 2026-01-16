--TEST--
Client: HEAD request (no body)
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

// ARRANGE: Create HEAD request using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
$request = $requestFactory->createRequest('HEAD', 'https://dummyjson.com/products/1');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $bodyContent = (string)$response->getBody();
    $bodyLength = strlen($bodyContent);
    $hasContentType = $response->hasHeader('Content-Type');

    // ASSERT: Verify HEAD returns headers but no body
    var_dump($response->getStatusCode() === 200);
    var_dump($hasContentType);
    var_dump($bodyLength === 0);
    echo "HEAD request successful\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
HEAD request successful
