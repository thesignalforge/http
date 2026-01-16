<?php
/**
 * Signalforge HTTP Extension
 * RequestException.stub.php - IDE stub for RequestException class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * Exception thrown when a request cannot be sent due to invalid request data.
 *
 * This includes:
 * - Invalid request format
 * - Missing required request data
 * - Request extraction failures
 *
 * @implements \Psr\Http\Client\RequestExceptionInterface
 */
class RequestException extends HttpException implements \Psr\Http\Client\RequestExceptionInterface
{
    /**
     * Returns the request that caused the exception.
     *
     * @return \Psr\Http\Message\RequestInterface
     */
    public function getRequest(): \Psr\Http\Message\RequestInterface {}
}
