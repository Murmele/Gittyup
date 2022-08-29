declare -a branches=("fetch_annotated_tags" "blame_abort" "disableRenameDetection" "checkout_deletion_notification" "diff_checkout" "fix_push_callback_issues" "disable_mmap" "context_line_accessor´" "disableRenameDetection" "submodule" "hash" "callback_connect_disconnect" "blame_buffer" "libgit2_includes_public")

cd libgit2

git remote remove origin
git remote remove upstream
git remote add origin git@github.com:Murmele/libgit2.git
git remote add upstream https://github.com/libgit2/libgit2.git

git fetch --all

for branch in ${branches[@]}; do
	echo $branch
	git checkout -B $branch "origin/$branch"
	git rebase upstream/main
done

git checkout Gittyup
git reset --hard upstream/main

for branch in ${branches[@]}; do
	git merge --no-edit $branch
done

git push --force origin

echo "Script finished. Check if all branches are rebased correctly and push them to origin!"
