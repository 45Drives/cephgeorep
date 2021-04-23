#pragma once

#include <vector>
#include <string>

class Syncer;
class File;

class SyncProcess{
private:
	int id_;
	/* Integral ID for each process to be used while printing
	 * log messages.
	 */
	pid_t pid_;
	/* PID of process.
	 */
	size_t start_arg_sz_;
	/* Number of bytes contained in the environment and in the
	 * executable path and flags.
	 */
	size_t max_arg_sz_;
	/* Maximum number of bytes allowed in environment and argv of called
	 * process. Roughly 1/4 of stack.
	 */
	size_t curr_arg_sz_;
	/* Keeps track of bytes in argument vector.
	 */
	uintmax_t max_bytes_sz_;
	/* Number of bytes of files on disk for each process to sync.
	 * Equal to total bytes divided by number of processes.
	 */
	uintmax_t curr_bytes_sz_;
	/* Keeps track of payload size.
	 */
	std::string exec_bin_;
	/* File syncing program name.
	 */
	std::string exec_flags_;
	/* Flags and extra args for program.
	 */
	std::vector<std::string>::iterator &destination_;
	/* [[<user>@]<host>:][<destination path>]
	 */
	std::string sending_to_;
	/* For printing failures after iterator changes.
	 */
	std::vector<File>::reverse_iterator file_itr_;
	/* List of files to sync.
	 */
public:
	SyncProcess(Syncer *parent, uintmax_t max_bytes_sz, std::vector<File> &queue);
	/* Constructor. Grabs members from parent pointer.
	 */
	~SyncProcess();
	/* Destructor. Frees memory in garbage vector before returning.
	 */
	int id() const;
	/* Return ID of sync process.
	 */
	pid_t pid() const;
	/* Return PID of sync process.
	 */
	void set_id(int id);
	/* Assign integral ID.
	 */
	uintmax_t payload_sz(void) const;
	/* Return number of bytes in file payload.
	 */
	uintmax_t payload_count(void) const;
	/* Return number of files in payload.
	 */
	void add(const File &file);
	/* Add one file to the payload.
	 * Incrememnts curr_arg_sz_ and curr_bytes_sz_ accordingly.
	 */
	bool full_test(const File &file) const;
	/* Returns true if curr_arg_sz_ or curr_bytes_sz_ exceed the maximums
	 * if file were to be added.
	 */
	bool large_file(const File &file) const;
	/* Check if file size is larger than maximum payload by itself.
	 * If this wasn't checked, large files would never sync.
	 */
	void consume(std::vector<File> &queue);
	/* Pop files from queue and push into files_ vector until full.
	 */
	void consume_one(std::vector<File> &queue);
	/* Only pop and push one file.
	 */
	void sync_batch(void);
	/* Fork and execute sync program with file batch.
	 */
	void clear_file_list(void);
	/* Clear files_ and reset curr_arg_sz_ and curr_bytes_sz_ to
	 * start_arg_sz_ and 0, respectively.
	 */
	bool payload_empty(void) const;
	/* Returns files_.empty().
	 */
	const std::string &destination(void) const;
	/* Returns sending_to_.
	 */
};
