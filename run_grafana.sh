podman run -d --name prometheus -p 9090:9090 -v ./config/prometheus.yml:/etc/prometheus/prometheus.yml harbor.bigdata.seisys.cn:8443/prom/prometheus

podman run -d --name=grafana -p 13000:3000  -e "GF_AUTH_PROXY_ENABLED=true"  -e "GF_AUTH_ANONYMOUS_ENABLED=true" -e "GF_SECURITY_ALLOW_EMBEDDING=true"  harbor.bigdata.seisys.cn:8443/prom/grafana:10.2.2
