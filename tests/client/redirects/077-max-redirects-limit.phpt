--TEST--
Client: Max redirect limit handling
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
use Signalforge\NativeHttp\NetworkException;

// ARRANGE: Create client with redirect settings using PSR-17 factory
// Use a smaller max_redirects than the redirect chain
$client = new Client(['follow_redirects' => true, 'max_redirects' => 2, 'timeout' => 10]);
$requestFactory = new RequestFactory();
// Request 5 redirects but only allow 2 - should fail
$request = $requestFactory->createRequest('GET', 'https://httpbin.org/redirect/5');

// ACT: Send the request
try {
    $response = $client->sendRequest($request);
    // If max_redirects is reached, curl should throw an error
    // Some curl versions might still return the redirect response
    $statusCode = $response->getStatusCode();
    var_dump($statusCode >= 300 && $statusCode < 400);
    echo "Redirect limit handled\n";
} catch (NetworkException $e) {
    // ASSERT: Too many redirects exception
    var_dump(true);
    echo "Redirect limit handled\n";
} catch (\Exception $e) {
    echo "Unexpected error: " . $e->getMessage() . "\n";
}
?>
--EXPECT--
bool(true)
Redirect limit handled
