--TEST--
signalforge_http: Response PSR-7 ResponseInterface status code validation
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Response;

// ACT: Test withStatus with valid status codes (all 100-599 ranges)
$response = Response::create();

// Test 1xx range
$infoResponse = $response->withStatus(100, 'Continue');
var_dump($infoResponse->getStatusCode() === 100);
var_dump($infoResponse->getReasonPhrase() === 'Continue');

// Test 2xx range
$successResponse = $response->withStatus(200, 'OK');
var_dump($successResponse->getStatusCode() === 200);
var_dump($successResponse->getReasonPhrase() === 'OK');

// Test 3xx range
$redirectResponse = $response->withStatus(302, 'Found');
var_dump($redirectResponse->getStatusCode() === 302);
var_dump($redirectResponse->getReasonPhrase() === 'Found');

// Test 4xx range
$clientErrorResponse = $response->withStatus(404, 'Not Found');
var_dump($clientErrorResponse->getStatusCode() === 404);
var_dump($clientErrorResponse->getReasonPhrase() === 'Not Found');

// Test 5xx range
$serverErrorResponse = $response->withStatus(500, 'Internal Server Error');
var_dump($serverErrorResponse->getStatusCode() === 500);
var_dump($serverErrorResponse->getReasonPhrase() === 'Internal Server Error');

// Test custom reason phrases
$customResponse = $response->withStatus(418, "I'm a teapot");
var_dump($customResponse->getStatusCode() === 418);
var_dump($customResponse->getReasonPhrase() === "I'm a teapot");

// Test empty reason phrase (should use default)
$emptyReasonResponse = $response->withStatus(404, '');
var_dump($emptyReasonResponse->getStatusCode() === 404);
var_dump($emptyReasonResponse->getReasonPhrase() === 'Not Found'); // Should use default

// Test very long reason phrases
$longReason = str_repeat('x', 1000);
$longReasonResponse = $response->withStatus(200, $longReason);
var_dump($longReasonResponse->getStatusCode() === 200);
var_dump($longReasonResponse->getReasonPhrase() === $longReason);

// Test reason phrases with special characters
$specialReasonResponse = $response->withStatus(200, 'Reason with spaces & symbols: !@#$%^&*()');
var_dump($specialReasonResponse->getStatusCode() === 200);
var_dump($specialReasonResponse->getReasonPhrase() === 'Reason with spaces & symbols: !@#$%^&*()');

// ACT: Test withStatus with invalid status codes
$invalidStatuses = [
    0, 99, // Below valid range
    600, 999, // Above valid range
    -1, -100, // Negative
    '200', // String
    200.5, // Float
    null, // Null
];

foreach ($invalidStatuses as $status) {
    try {
        $response->withStatus($status);
        var_dump(false); // Should not reach here
    } catch (InvalidArgumentException $e) {
        var_dump(true); // Exception correctly thrown
    } catch (Exception $e) {
        var_dump(false); // Wrong exception type
    }
}

// ASSERT: Immutability maintained
var_dump($response->getStatusCode() === 200);
var_dump($response->getReasonPhrase() === 'OK');
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(false)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
