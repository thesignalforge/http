<?php
/**
 * Signalforge HTTP Extension
 * NetworkException.stub.php - IDE stub for NetworkException class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * Exception thrown when a network error occurs during request execution.
 *
 * This includes:
 * - Connection failures
 * - DNS resolution failures
 * - Timeout errors
 * - SSL/TLS errors
 *
 * @implements \Psr\Http\Client\NetworkExceptionInterface
 */
class NetworkException extends HttpException implements \Psr\Http\Client\NetworkExceptionInterface
{
    /**
     * Returns the request that caused the exception.
     *
     * @return \Psr\Http\Message\RequestInterface
     */
    public function getRequest(): \Psr\Http\Message\RequestInterface {}
}
