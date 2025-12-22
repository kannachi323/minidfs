//
//  FileGridView.swift
//  homelib-ios
//
//  Created by Matthew Chen on 7/20/25.
//

import SwiftUI

struct FileGridView: View {
    @ObservedObject var viewModel: FileGridViewModel
    
    let columns = [
        GridItem(.flexible()),
        GridItem(.flexible())
    ]
    var body: some View {
        ScrollView {
            LazyVGrid(columns: columns, spacing: 30) {
                ForEach(viewModel.files) { file in
                    VStack(spacing: 10) {
                        Image(systemName: file.isDir ? "folder.fill" : "doc.fill")
                            .resizable()
                            .scaledToFit()
                            .frame(maxWidth: 64, maxHeight: 64)
                            .foregroundColor(file.isDir ? .yellow : .blue)
                        
                        Text(file.name)
                            .font(.caption)
                            .multilineTextAlignment(.center)
                    }
                    .frame(maxWidth: .infinity)
                    .onTapGesture {
                        if (file.isDir) {
                            viewModel.currentPath = file.path
                            viewModel.fetchFiles(path: viewModel.currentPath)
                        }
                        
                      }
                }
            }
        }
    }
}

#Preview {
    let viewModel = FileGridViewModel()
    viewModel.fetchFiles(path: "/")
    return FileGridView(viewModel: viewModel)
}



