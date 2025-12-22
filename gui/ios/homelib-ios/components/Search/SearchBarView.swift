//
//  SearchBarView.swift
//  homelib-ios
//
//  Created by Matthew Chen on 7/20/25.
//
import SwiftUI


struct SearchBarView : View {
    @Binding var searchText : String
    @Binding var isOpen : Bool
    
    var body: some View {

        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Image(systemName: "line.3.horizontal")
                    .foregroundColor(.gray)
                    .onTapGesture {
                        isOpen.toggle()
                        if (isOpen) {
                            print("opened")
                        } else {
                            print("closed")
                        }
                    
                        
                    }
                
                TextField("Search in HomeLib", text: $searchText)
                    .textFieldStyle(PlainTextFieldStyle())
                    .autocorrectionDisabled(true)
                    .foregroundColor(.white)
                    
            }
            .frame(maxWidth: .infinity, maxHeight: 25)
            .padding(10)
            .cornerRadius(10)
            .overlay(
                RoundedRectangle(cornerRadius: 10)
                    .stroke(Color.white, lineWidth: 2)
            )
            
        }
  
    }
}

struct SearchBarPreview : View {
    @State private var text = ""
    @State private var isDrawerOpen = false
    
    var body : some View {
        SearchBarView(searchText: $text, isOpen: $isDrawerOpen)
    }
}

#Preview {
    SearchBarPreview()
}
