declare -a branches=("origin/fetch_annotated_tags" "origin/blame_abort" "origin/disableRenameDetection" "origin/checkout_deletion_notification" "origin/diff_checkout" "origin/fix_push_callback_issues" "origin/disable_mmap" "origin/context_line_accessor´" "origin/disableRenameDetection" "origin/submodule" "origin/hash" "origin/callback_connect_disconnect" "origin/blame_buffer" "origin/libgit2_includes_public")

cd libgit2

for branch in ${branches[@]}; do
	echo $branch
	git checkout $branch
	git rebase upstream/main
done

git checkout Gittyup
git reset --hard upstream/main

for branch in ${branches[@]}; do
	git merge --no-edit $branch
done

echo "Script finished. Check if all branches are rebased correctly and push them to origin!"