until Dir.exist?("chef") do
  Dir.chdir("..")
end
pwd = Dir.pwd
chef_repo_path "#{pwd}/chef"
cookbook_path [ "#{pwd}/chef/cookbooks", "#{pwd}/chef/site-cookbooks" ]
