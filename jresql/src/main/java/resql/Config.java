    /*
 * BSD-3-Clause
 *
 * Copyright 2021 Ozan Tezcan
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
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

package resql;

import java.util.ArrayList;
import java.util.List;

public class Config {
    String clientName = null;
    String clusterName = "cluster";
    String outgoingAddr = null;
    int outgoingPort = 0;
    int timeoutMillis = Integer.MAX_VALUE;
    List<String> urls = new ArrayList<>();

    /**
     * Create new Config instance
     *
     * Default timeout is Integer.MAX_VALUE.
     * Default url is 127.0.0.1:7600
     *
     */
    public Config() {
        urls.add("tcp://127.0.0.1:7600");
    }

    /**
     * Create new Config instance
     *
     * @param clientName    client name
     * @param clusterName   cluster name must match with cluster name of servers
     * @param timeoutMillis timeout milliseconds
     * @param urls          urls
     */
    public Config(String clientName, String clusterName, int timeoutMillis,
            List<String> urls) {
        this.clientName = clientName;
        this.clusterName = clusterName;
        this.timeoutMillis = timeoutMillis;
        this.urls = urls;
    }

    public Config setClientName(String clientName) {
        this.clientName = clientName;
        return this;
    }

    public Config setClusterName(String clusterName) {
        this.clusterName = clusterName;
        return this;
    }

    public Config setTimeoutMillis(int timeoutMillis) {
        this.timeoutMillis = timeoutMillis;
        return this;
    }

    public Config setUrls(List<String> urls) {
        this.urls = urls;
        return this;
    }

    public Config setOutgoingAddr(String outgoingAddr) {
        this.outgoingAddr = outgoingAddr;
        return this;
    }

    public Config setOutgoingPort(int outgoingPort) {
        this.outgoingPort = outgoingPort;
        return this;
    }
}
