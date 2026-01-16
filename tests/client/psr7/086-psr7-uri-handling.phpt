--TEST--
Client: PSR-7 URI with query and fragment
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
use Signalforge\NativeHttp\Uri;

// ARRANGE: Create request with PSR-7 URI using PSR-17 factory
$client = new Client(['timeout' => 10]);
$requestFactory = new RequestFactory();
// Use Uri::fromString() factory method (Uri has no constructor)
$uri = Uri::fromString('https://dummyjson.com/products/search?q=phone&limit=5');
$request = $requestFactory->createRequest('GET', $uri);

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    $body = (string)$response->getBody();
    $data = json_decode($body, true);

    // ASSERT: Verify URI was handled correctly
    var_dump($response->getStatusCode() === 200);
    var_dump(isset($data['products']));
    var_dump(isset($data['limit']) && $data['limit'] === 5);
    echo "PSR-7 URI handled correctly\n";
} catch (\Exception $e) {
    echo "Error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
PSR-7 URI handled correctly
