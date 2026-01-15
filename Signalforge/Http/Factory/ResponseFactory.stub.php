<?php
/**
 * Signalforge HTTP Extension
 * ResponseFactory.stub.php - IDE stub for ResponseFactory class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * PSR-17 compliant response factory.
 *
 * Creates ResponseInterface instances.
 *
 * @final
 * @implements \Psr\Http\Message\ResponseFactoryInterface
 */
final class ResponseFactory implements \Psr\Http\Message\ResponseFactoryInterface
{
    /**
     * Create a new response.
     *
     * @param int $code HTTP status code (default: 200)
     * @param string $reasonPhrase The reason phrase to associate with the status code
     * @return \Psr\Http\Message\ResponseInterface
     * @throws \InvalidArgumentException For invalid status codes (outside 100-599)
     */
    public function createResponse(int $code = 200, string $reasonPhrase = ''): \Psr\Http\Message\ResponseInterface {}
}
