/*
 * MIT License
 *
 * Copyright (c) 2021 Resql Authors
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

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.StandardSocketOptions;
import java.net.URI;
import java.net.URISyntaxException;
import java.nio.channels.SocketChannel;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicBoolean;


class Client implements Resql {
    private final String clientName;
    private final String clusterName;
    private final int timeout;
    private long seq = 0;
    private boolean connected = false;
    private boolean hasStatement;
    private AtomicBoolean shutdown = new AtomicBoolean();

    private SocketChannel sock;
    private int urlIndex = 0;
    private long urlsTerm = 0;
    private List<URI> urls = new ArrayList<>();

    private final Result result = new Result();
    private final RawBuffer resp = new RawBuffer(32 * 1024);
    private final RawBuffer req = new RawBuffer(32 * 1024);


    public Client(Config config) {
        if (config.clusterName == null) {
            throw new ResqlException("Cluster name cannot be null");
        }

        if (config.timeoutMillis < 2000) {
            throw new ResqlException("Timeout must be greater than 2000");
        }

        if (config.urls == null || config.urls.size() == 0) {
            throw new ResqlException("At least one URL must be specified");
        }

        String n = config.clientName;
        clientName = n != null ? n : UUID.randomUUID().toString();
        clusterName = config.clusterName;
        timeout = config.timeoutMillis;

        for (String s : config.urls) {
            try {
                urls.add(new URI(s));
            } catch (URISyntaxException e) {
                throw new ResqlException(e);
            }
        }

        long ts = System.currentTimeMillis();

        while (!connected && timeout > System.currentTimeMillis() - ts) {
            try {
                connect();
            } catch (ResqlFatalException e) {
                clear();
                throw new ResqlException(e.getCause());
            } catch (ResqlException ignored) {
            }
        }

        if (!connected) {
            throw new ResqlException("Failed to connect");
        }

        clear();
    }

    public void cancel() {
        
    }

    public void shutdown() {
        resp.clear();

        if (!connected) {
            return;
        }

        Msg.encodeDisconnectReq(resp, (byte) 0, 0);

        try {
            sock.write(resp.backend());
        } catch (IOException e) {
            throw new ResqlException(e);
        } finally {
            disconnect();
        }
    }

    private void connect() {
        boolean ok;
        URI uri = urls.get(urlIndex);
        urlIndex = (urlIndex + 1) % urls.size();

        disconnect();

        try {
            sock = SocketChannel.open();
            sock.setOption(StandardSocketOptions.TCP_NODELAY, true);
            sock.setOption(StandardSocketOptions.SO_RCVBUF, 32 * 1024);
            sock.setOption(StandardSocketOptions.SO_SNDBUF, 32 * 1024);
            sock.socket().setSoTimeout(2000);
        } catch (IOException e) {
            disconnect();
            throw new ResqlFatalException(e);
        }

        try {
            InetSocketAddress addr = new InetSocketAddress(uri.getHost(),
                                                           uri.getPort());
            ok = sock.connect(addr);
            if (!ok) {
                return;
            }

            resp.clear();
            Msg.encodeConnectReq(resp, clusterName, clientName);

            sock.write(resp.backend());
            if (resp.remaining() != 0) {
                return;
            }

            resp.clear();
            if (sock.read(resp.backend()) <= 0) {
                return;
            }
        } catch (IOException e) {
            disconnect();
            throw new ResqlException(e);
        }

        resp.flip();
        if (resp.getInt() != resp.limit()) {
            return;
        }

        if (resp.get() != Msg.CONNECT_RESP) {
            return;
        }

        int rc = resp.get();
        long seq = resp.getLong();
        long term = resp.getLong();
        String nodes = resp.getString();

        if (term > urlsTerm) {
            List<URI> latest = new ArrayList<>();
            String[] parts = nodes.split(" ");
            for (String s : parts) {
                try {
                    latest.add(new URI(s));
                } catch (URISyntaxException ignore) {

                }
            }

            if (latest.size() != 0) {
                urls = latest;
                urlsTerm = term;
                urlIndex = 0;
            }
        }

        if (rc != Msg.MSG_OK) {
            return;
        }

        if (!req.hasRemaining()) {
            this.seq = seq;
        } else {
            if (seq != this.seq && seq != this.seq - 1) {
                this.seq = seq;
                throw new ResqlFatalException(
                        "Client session does not exist");
            }
        }

        connected = true;
    }

    @Override
    public void clear() {
        req.clear();
        Msg.reserveClientReqHeader(req);
    }

    private void disconnect() {
        if (sock != null) {
            try {
                sock.close();
                sock = null;
            } catch (IOException ignored) {
            }
        }
        connected = false;
    }

    private void sendRequest() {
        long ts = System.currentTimeMillis();

        req.flip();
        resp.clear();

        while (timeout > System.currentTimeMillis() - ts) {
            try {
                if (!connected) {
                    connect();
                }
            } catch (ResqlFatalException e) {
                clear();
                throw new ResqlException(e.getCause());
            } catch (ResqlException ignored) {
            }

            if (!connected) {
                continue;
            }

            try {
                int b = sock.write(req.backend());
                if (b == 0 || req.remaining() != 0) {
                    continue;
                }

                req.rewind();
            } catch (IOException e) {
                disconnect();
                continue;
            }

            while (timeout > System.currentTimeMillis() - ts) {
                try {
                    int b = sock.read(resp.backend());
                    if (b <= 0) {
                        connected = false;
                        continue;
                    }
                } catch (IOException e) {
                    connected = false;
                    break;
                }

                resp.flip();

                int remaining = Msg.remaining(resp);
                if (remaining == 0) {
                    return;
                }

                if (remaining > resp.cap()) {
                    int limit = resp.limit();
                    resp.limit(resp.cap());
                    resp.position(limit);
                    resp.reserve(remaining - resp.position());
                } else {
                    int limit = resp.limit();
                    resp.limit(resp.cap());
                    resp.position(limit);
                }
            }
        }

        throw new ResqlException("Request failed");
    }

    @Override
    public void put(String statement) {
        if (hasStatement) {
            req.put(Msg.FLAG_END);
        }

        hasStatement = true;

        req.put(Msg.FLAG_STMT);
        req.putString(statement);
    }

    @Override
    public void put(PreparedStatement statement) {
        if (hasStatement) {
            req.put(Msg.FLAG_END);
        }

        hasStatement = true;

        req.put(Msg.FLAG_STMT_ID);
        req.putLong(((Prepared) statement).getId());
    }

    @Override
    public PreparedStatement prepare(String sql) {
        if (sql == null) {
            throw new ResqlSQLException("Prepare sql cannot be null.");
        }

        if (hasStatement) {
            throw new ResqlSQLException(
                    "Statement prepare must be a single operation.");
        }

        req.put(Msg.FLAG_STMT_PREPARE);
        req.putString(sql);
        req.put(Msg.FLAG_END);

        seq++;
        Msg.encodeClientReq(req, false, seq);

        sendRequest();
        clear();

        if (resp.get() != Msg.CLIENT_RESP) {
            throw new ResqlException("Disconnected.");
        }

        if (resp.get() == Msg.FLAG_ERROR) {
            throw new ResqlSQLException(resp.getString());
        }

        return new Prepared(resp.getLong(), sql);
    }

    @Override
    public void delete(PreparedStatement statement) {
        if (hasStatement) {
            throw new ResqlSQLException(
                    "Delete must be a single operation");
        }

        req.put(Msg.FLAG_STMT_DEL_PREPARED);
        req.putLong(((Prepared) statement).getId());
        req.put(Msg.FLAG_END);

        seq++;
        Msg.encodeClientReq(req, false, seq);

        sendRequest();
        clear();

        if (resp.get() != Msg.CLIENT_RESP) {
            throw new ResqlException("Disconnected");
        }

        if (resp.get() == Msg.FLAG_ERROR) {
            throw new ResqlSQLException(resp.getString());
        }
    }

    @Override
    public ResultSet execute(boolean readonly) {
        if (!hasStatement) {
            throw new ResqlSQLException("Put a statement first.");
        }

        req.put(Msg.FLAG_END);
        hasStatement = false;

        if (!readonly) {
            seq++;
        }

        Msg.encodeClientReq(req, readonly, seq);
        sendRequest();
        clear();

        if (resp.get() != Msg.CLIENT_RESP) {
            throw new ResqlException("Disconnected.");
        }

        if (resp.get() == Msg.FLAG_ERROR) {
            throw new ResqlSQLException(resp.getString());
        }

        result.setBuf(resp);

        return result;
    }


    private void bind(Object value) {
        if (value == null) {
            req.put(Msg.PARAM_NULL);
        } else if (value instanceof Integer) {
            req.put(Msg.PARAM_INTEGER);
            req.putLong((Integer) value);
        } else if (value instanceof Long) {
            req.put(Msg.PARAM_INTEGER);
            req.putLong((Long) value);
        } else if (value instanceof Double) {
            req.put(Msg.PARAM_FLOAT);
            req.putDouble((Double) value);
        } else if (value instanceof String) {
            req.put(Msg.PARAM_TEXT);
            req.putString((String) value);
        } else if (value instanceof byte[]) {
            req.put(Msg.PARAM_BLOB);
            req.putBlob((byte[]) value);
        }
    }


    @Override
    public void bind(int index, Object value) {
        if (!hasStatement) {
            throw new ResqlSQLException("Put statement first.");
        }

        req.put(Msg.PARAM_INDEX);
        req.putInt(index);
        bind(value);
    }

    @Override
    public void bind(String param, Object value) {
        if (!hasStatement) {
            throw new ResqlSQLException("Put statement first.");
        }

        req.put(Msg.PARAM_NAME);
        req.putString(param);
        bind(value);
    }
}
