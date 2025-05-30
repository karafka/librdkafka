/*
 * librdkafka - Apache Kafka C library
 *
 * Copyright (c) 2016-2022, Magnus Edenhill
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "test.h"

/* Typical include path would be <librdkafka/rdkafka.h>, but this program
 * is built from within the librdkafka source tree and thus differs. */
#include "rdkafka.h" /* for Kafka driver */


/**
 * Issue #597: increase fetch.message.max.bytes until large messages can
 *             be fetched.
 *
 * General idea:
 *  - Produce 1000 small messages < MAX_BYTES
 *  - Produce 1000 large messages > MAX_BYTES
 *  - Create consumer with fetch.message.max.bytes=MAX_BYTES
 *  - Consume from beginning
 *  - All messages should be received.
 */


int main_0041_fetch_max_bytes(int argc, char **argv) {
        const char *topic   = test_mk_topic_name(__FUNCTION__, 1);
        const int partition = 0;
        const int msgcnt    = 2 * 1000;
        const int MAX_BYTES = 100000;
        uint64_t testid;
        rd_kafka_conf_t *conf;
        rd_kafka_t *rk;
        rd_kafka_topic_t *rkt;

        test_conf_init(NULL, NULL, 60);

        testid = test_id_generate();
        rk     = test_create_producer();
        rkt    = test_create_producer_topic(rk, topic, NULL);
        test_wait_topic_exists(rk, topic, 5000);

        test_produce_msgs(rk, rkt, testid, partition, 0, msgcnt / 2, NULL,
                          MAX_BYTES / 10);
        test_produce_msgs(rk, rkt, testid, partition, msgcnt / 2, msgcnt / 2,
                          NULL, MAX_BYTES * 5);

        rd_kafka_topic_destroy(rkt);
        rd_kafka_destroy(rk);

        TEST_SAY("Creating consumer\n");
        test_conf_init(&conf, NULL, 0);

        test_conf_set(conf, "fetch.message.max.bytes",
                      tsprintf("%d", MAX_BYTES));

        /* This test may be slower when running with SSL or Helgrind,
         * restart the timeout. */
        test_timeout_set(60);

        rk  = test_create_consumer(NULL, NULL, conf, NULL);
        rkt = rd_kafka_topic_new(rk, topic, NULL);

        test_consumer_start("CONSUME", rkt, partition,
                            RD_KAFKA_OFFSET_BEGINNING);
        test_consume_msgs("CONSUME", rkt, testid, partition, TEST_NO_SEEK, 0,
                          msgcnt, 1);
        test_consumer_stop("CONSUME", rkt, partition);

        rd_kafka_topic_destroy(rkt);
        rd_kafka_destroy(rk);

        return 0;
}
