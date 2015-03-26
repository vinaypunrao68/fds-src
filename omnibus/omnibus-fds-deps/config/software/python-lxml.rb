name "python-lxml"
default_version "3.4.2"

dependency "pip"
dependency "libxml2"
dependency "libxslt1"

build do
  env = with_standard_compiler_flags(with_embedded_path)
  command "#{install_dir}/embedded/bin/pip install -I --build #{project_dir} lxml==#{version}", env: env
end
