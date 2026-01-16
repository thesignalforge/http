--TEST--
Client: Thread pool error propagation from workers
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
/**
 * Tests that errors from worker threads are properly propagated back
 * to the main thread and handled through callbacks or exceptions.
 */
use Signalforge\NativeHttp\Client;
use Signalforge\NativeHttp\HttpRequestPool;
use Signalforge\NativeHttp\RequestFactory;

$requestFactory = new RequestFactory();

echo "=== Test 1: Mix of success and HTTP errors ===\n";
$client = new Client(['pool_size' => 4, 'timeout' => 30]);
$pool = new HttpRequestPool($client, 10);

$results = ['success' => 0, 'errors' => 0];

// Add successful requests
for ($i = 1; $i <= 3; $i++) {
    $request = $requestFactory->createRequest('GET', 'https://dummyjson.com/products/' . $i);
    $pool->add($request,
        function($response) use (&$results) {
            if ($response->getStatusCode() >= 200 && $response->getStatusCode() < 400) {
                $results['success']++;
            }
        },
        function($error) use (&$results) {
            $results['errors']++;
        }
    );
}

// Add requests that return HTTP errors (still success callbacks - HTTP status != exception)
$pool->add(
    $requestFactory->createRequest('GET', 'https://dummyjson.com/http/500'),
    function($response) use (&$results) {
        // HTTP 500 still returns a response
        $results['success']++;
    },
    function($error) use (&$results) {
        $results['errors']++;
    }
);

$pool->add(
    $requestFactory->createRequest('GET', 'https://dummyjson.com/http/404'),
    function($response) use (&$results) {
        // HTTP 404 still returns a response
        $results['success']++;
    },
    function($error) use (&$results) {
        $results['errors']++;
    }
);

$responses = $pool->wait();
var_dump(count($responses) >= 4); // Should have responses for all requests
var_dump($results['success'] >= 4); // Success callbacks for successful fetches
echo "Mixed requests handled\n";

echo "\n=== Test 2: Network errors with callbacks ===\n";
$client2 = new Client(['pool_size' => 2, 'timeout' => 5]); // Short timeout
$pool2 = new HttpRequestPool($client2, 10);

$networkResults = ['success' => 0, 'network_errors' => 0];

// Valid request
$pool2->add(
    $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1'),
    function($response) use (&$networkResults) {
        $networkResults['success']++;
    },
    function($error) use (&$networkResults) {
        $networkResults['network_errors']++;
    }
);

// Invalid host (should trigger network error)
$pool2->add(
    $requestFactory->createRequest('GET', 'http://invalid.invalid.example/test'),
    function($response) use (&$networkResults) {
        $networkResults['success']++;
    },
    function($error) use (&$networkResults) {
        $networkResults['network_errors']++;
    }
);

$responses = $pool2->wait();
var_dump($networkResults['success'] >= 1);
var_dump($networkResults['network_errors'] >= 1);
echo "Network errors propagated via callbacks\n";

echo "\n=== Test 3: Error isolation between requests ===\n";
$client3 = new Client(['pool_size' => 4, 'timeout' => 10]);
$pool3 = new HttpRequestPool($client3, 10);

$isolation = ['request_1_ok' => false, 'request_2_status' => 0, 'request_3_ok' => false];

// Request 1 - should succeed
$pool3->add(
    $requestFactory->createRequest('GET', 'https://dummyjson.com/products/1'),
    function($response) use (&$isolation) {
        $isolation['request_1_ok'] = $response->getStatusCode() === 200;
    },
    function($error) use (&$isolation) {}
);

// Request 2 - HTTP 500 error (reliable way to test error handling)
$pool3->add(
    $requestFactory->createRequest('GET', 'https://dummyjson.com/http/500'),
    function($response) use (&$isolation) {
        // 500 still returns a response
        $isolation['request_2_status'] = $response->getStatusCode();
    },
    function($error) use (&$isolation) {}
);

// Request 3 - should still succeed (not affected by request 2's HTTP error)
$pool3->add(
    $requestFactory->createRequest('GET', 'https://dummyjson.com/products/2'),
    function($response) use (&$isolation) {
        $isolation['request_3_ok'] = $response->getStatusCode() === 200;
    },
    function($error) use (&$isolation) {}
);

$responses = $pool3->wait();
var_dump($isolation['request_1_ok']);
var_dump($isolation['request_2_status'] === 500);
var_dump($isolation['request_3_ok']);
echo "Errors isolated between requests\n";

echo "\nAll error propagation tests passed!\n";
?>
--EXPECT--
=== Test 1: Mix of success and HTTP errors ===
bool(true)
bool(true)
Mixed requests handled

=== Test 2: Network errors with callbacks ===
bool(true)
bool(true)
Network errors propagated via callbacks

=== Test 3: Error isolation between requests ===
bool(true)
bool(true)
bool(true)
Errors isolated between requests

All error propagation tests passed!
