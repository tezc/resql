package resql;

import java.util.ArrayList;
import java.util.List;

public class Config {
    String clientName = null;
    String clusterName = "cluster";
    int timeoutMillis = Integer.MAX_VALUE;
    List<String> urls = new ArrayList<>();

    public Config() {
        urls.add("tcp://127.0.0.1:8080");
    }

    public Config(String clientName, String clusterName, int timeoutMillis,
                  List<String> urls) {
        this.clientName = clientName;
        this.clusterName = clusterName;
        this.timeoutMillis = timeoutMillis;
        this.urls = urls;
    }

    public void setClientName(String clientName) {
        this.clientName = clientName;
    }

    public void setClusterName(String clusterName) {
        this.clusterName = clusterName;
    }

    public void setTimeoutMillis(int timeoutMillis) {
        this.timeoutMillis = timeoutMillis;
    }

    public void setUrls(List<String> urls) {
        this.urls = urls;
    }
}
