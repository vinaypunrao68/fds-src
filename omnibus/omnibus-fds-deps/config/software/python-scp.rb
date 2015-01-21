name "python-scp"
default_version "0.8.0"

dependency "pip"

build do
  command "#{install_dir}/embedded/bin/pip install -I --build #{project_dir} scp==#{version}"
end
