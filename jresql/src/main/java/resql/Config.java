    /*
 * MIT License
 *
 * Copyright (c) 2021 Ozan Tezcan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
