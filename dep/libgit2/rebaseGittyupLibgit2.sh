declare -a branches=("fetch_annotated_tags" "blame_abort" "disableRenameDetection" "checkout_deletion_notification" "diff_checkout" "fix_push_callback_issues" "disable_mmap" "context_line_accessor´" "disableRenameDetection" "hash" "callback_connect_disconnect" "libgit2_includes_public" "local_libssh2")

cd libgit2

git remote remove origin
git remote remove upstream
git remote add origin https://github.com/Murmele/libgit2.git
git remote add upstream https://github.com/libgit2/libgit2.git

git fetch --all

for branch in ${branches[@]}; do
	echo $branch
	git checkout -B $branch "origin/$branch"
	RESULT=$?
	if [ $RESULT -gt 0 ]; then
	  echo "Unable to checkout branch: $branch : Exitcode: $RESULT"
	  exit 1
	fi
	git rebase upstream/main
	RESULT=$?
	if [ $RESULT -gt 0 ]; then
	  echo "Unable to rebase branch: $branch : Exitcode: $RESULT"
	  exit 1
	fi
done

git checkout Gittyup
git reset --hard upstream/main

for branch in ${branches[@]}; do
	git merge --no-edit $branch
	RESULT=$?
	if [ $RESULT -gt 0 ]; then
	  echo "Unable to merge branch: $branch : Exitcode: $RESULT"
	  exit 1
	fi
done

git push --force origin

echo "Script finished. Check if all branches are rebased correctly and push them to origin!"
