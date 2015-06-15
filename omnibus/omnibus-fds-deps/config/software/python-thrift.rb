name "python-thrift"
default_version "0.9.0"

dependency "pip"

build do
  command "#{install_dir}/embedded/bin/pip install -I --build #{project_dir} thrift==#{version}"
end
