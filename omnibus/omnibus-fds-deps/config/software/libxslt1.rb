name "libxslt1"
default_version "1.1.28"

version "1.1.28" do
  source md5: "9667bf6f9310b957254fdcf6596600b7"
end

dependency "libgcrypt"

source url: "http://archive.ubuntu.com/ubuntu/pool/main/libx/libxslt/libxslt_#{version}.orig.tar.gz"

relative_path "libxslt-#{version}"

build do
  env = with_standard_compiler_flags(with_embedded_path)

  patch source: 'fix_libxslt_path_for_libexslt.patch'

  command "./configure --prefix=#{install_dir}/embedded", env:env

  command "cd libxslt && make -j #{workers}", env:env
  sync "#{project_dir}/libxslt", "#{install_dir}/embedded/include/libxslt", exclude: ".libs"
  copy "#{project_dir}/libxslt/.libs/libxslt.*", "#{install_dir}/embedded/lib/"

  # This seemingly asinine - nay, entirely asinine - link is needed to make libexslt stop
  #  complaining about not finding libxslt in the right place. I've already burned too many
  #  hours trying to do it up right - the xslt devs decided that's just not an option
  mkdir "#{install_dir}/embedded/include/libxslt/.libs"
  link "#{install_dir}/embedded/lib/libxslt.so", "#{install_dir}/embedded/include/libxslt/.libs/libxslt.so"

  command "cd libexslt && make", env:env
  sync "#{project_dir}/libexslt", "#{install_dir}/embedded/include/libexslt", exclude: ".libs"
  copy "#{project_dir}/libexslt/.libs/libexslt.*", "#{install_dir}/embedded/lib/"
end
