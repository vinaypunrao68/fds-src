name "python-influxdb"
default_version "0.3.1"

dependency "pip"

build do
  command "#{install_dir}/embedded/bin/pip install -I --build #{project_dir} influxdb==#{version}"
end
