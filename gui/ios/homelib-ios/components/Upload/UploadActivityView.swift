//
//  UploadStatusView.swift
//  homelib-ios
//
//  Created by Matthew Chen on 7/21/25.
//


import SwiftUI
import UniformTypeIdentifiers

struct UploadFileView: View {
    @State private var showingFileImporter = false
    @State private var selectedFileURL: URL?

    var body: some View {
        VStack(spacing: 20) {
            if let url = selectedFileURL {
                Text("Selected file: \(url.lastPathComponent)")
                    .padding()
            } else {
                Text("No file selected")
                    .foregroundColor(.gray)
            }

            Button("Select File") {
                showingFileImporter = true
            }
            .fileImporter(
                isPresented: $showingFileImporter,
                allowedContentTypes: [.item], // allows any file type
                allowsMultipleSelection: false
            ) { result in
                switch result {
                case .success(let urls):
                    selectedFileURL = urls.first
                case .failure(let error):
                    print("Failed to select file: \(error.localizedDescription)")
                }
            }

            Button("Upload File") {
                uploadFile()
            }
            .disabled(selectedFileURL == nil)
        }
        .padding()
        .toolbar() {
            ToolbarItem(placement: .principal) {
                Text("Files")
                    .font(.title)
                    .bold()
                    .foregroundColor(.white)
            }
        }
        .preferredColorScheme(.dark)
    }

    func uploadFile() {
        guard let fileURL = selectedFileURL else { return }
        // Here you'd add your upload logic (e.g. POST file to your server)
        print("Uploading file: \(fileURL)")
    }
}

#Preview {
    UploadFileView()
}
